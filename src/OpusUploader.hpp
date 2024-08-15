#pragma once

#include <filesystem>
#include <sstream>
#include <iomanip>
#include <iostream>

#include <opusenc.h>

#include "AudioRecordClient.hpp"

struct SegmentEntry {
	int segmentIndex;
	std::filesystem::path segmentPath;
};

class OpusUploader {
	const std::filesystem::path outputDirectory;
	const std::string outputExt;
	const std::string outputPrefix;

	OggOpusComments *comments;
	OggOpusEnc *encoder;

	int segmentIndex;
	SegmentEntry ongoingSegment;
	std::vector<const SegmentEntry> segmentEntries;

	std::vector<std::string> destinationURLs;

	AudioRecordClient &audioRecordClient;
	AuthClient &authClient;

	static std::filesystem::path
	generateNextStreamPath(const std::filesystem::path &outputDirectory,
			       const std::string &outputExt,
			       const std::string &outputPrefix,
			       int segmentIndex)
	{
		using std::filesystem::path;
		std::ostringstream filenameStream;
		filenameStream << outputPrefix << " " << std::setfill('0')
			       << std::setw(6) << segmentIndex << "."
			       << outputExt;
		const path outputPath = outputDirectory / filenameStream.str();
		return outputPath;
	}

	void loadNewDestinations(int start, int count)
	{
		std::string idToken = authClient.getIdToken();
		const auto result = audioRecordClient.getDestinations(
			outputExt, outputPrefix, start, count, idToken);
		destinationURLs.insert(destinationURLs.end(),
				       result.destinations.begin(),
				       result.destinations.end());
	}

public:
	int destinationBatchCount = 200;
	int destinationBatchBackoff = 100;

	OpusUploader(std::filesystem::path _outputDirectory,
		     std::string _outputExt, std::string _outputPrefix,
		     int sampleRate, AudioRecordClient &_audioRecordClient,
		     AuthClient &_authClient)
		: outputDirectory(_outputDirectory),
		  outputExt(_outputExt),
		  outputPrefix(_outputPrefix),
		  audioRecordClient(_audioRecordClient),
		  authClient(_authClient)
	{
		using std::filesystem::path;

		comments = ope_comments_create();

		path outputPath(generateNextStreamPath(outputDirectory,
						       outputExt, outputPrefix,
						       segmentIndex));
		ongoingSegment = {segmentIndex, outputPath};
		std::filesystem::create_directories(outputPath.parent_path());

		int error;
		encoder = ope_encoder_create_file(outputPath.string().c_str(),
						  comments, sampleRate, 2, 0,
						  &error);
		if (!encoder) {
			obs_log(LOG_ERROR, "%s", ope_strerror(error));
			return;
		}
	}

	~OpusUploader(void)
	{
		ope_encoder_drain(encoder);
		ope_encoder_destroy(encoder);
		ope_comments_destroy(comments);
	}

	bool continueNewStream(void)
	{
		using std::filesystem::path;

		segmentIndex += 1;
		path outputPath(generateNextStreamPath(outputDirectory,
						       outputExt, outputPrefix,
						       segmentIndex));
		segmentEntries.push_back(ongoingSegment);
		ongoingSegment = {segmentIndex, outputPath};
		std::filesystem::create_directories(outputPath.parent_path());

		int error = ope_encoder_continue_new_file(
			encoder, outputPath.string().c_str(), comments);
		if (error) {
			obs_log(LOG_ERROR, "%s", ope_strerror(error));
			return false;
		}
		return true;
	}

	bool uploadPendingSegments(void)
	{
		if (destinationURLs.empty()) {
			loadNewDestinations(0, destinationBatchCount);
		}

		for (const SegmentEntry &segmentEntry : segmentEntries) {
			const int &index = segmentEntry.segmentIndex;
			const int count =
				static_cast<int>(destinationURLs.size());

			if (index < 0 || index >= count) {
				obs_log(LOG_ERROR, "Invalid segment sequence!");
				return false;
			}

			const auto &segmentPath = segmentEntry.segmentPath;
			const auto &uploadURL =
				destinationURLs[segmentEntry.segmentIndex];
			obs_log(LOG_INFO, "Uploading %s to %s",
				segmentPath.string().c_str(),
				uploadURL.c_str());

			if (index + destinationBatchBackoff >= count) {
				loadNewDestinations(count,
						    destinationBatchCount);
			}
		}

		segmentEntries.clear();

		return true;
	}

	void write(const float *pcm, int samplesPerChannel)
	{
		ope_encoder_write_float(encoder, pcm, samplesPerChannel);
	}
};
