#pragma once

#include <filesystem>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>

#include <opusenc.h>
#include <hash-library/md5.h>

#include "AudioRecordClient.hpp"
#include "Base64.hpp"

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
	std::vector<SegmentEntry> segmentEntries;

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

	static bool isLittleEndian()
	{
		int i = 1;
		return *((char *)&i) == 1;
	}

	std::string calculateMD5Hash(std::filesystem::path path) const
	{
    MD5 md5;
		std::ifstream ifs(path, std::ios::binary);
    char buf[8192];
		while (!ifs.eof()) {
			ifs.read(buf, sizeof(buf));
      md5.add(buf, ifs.gcount());
		}

    unsigned char digest[MD5::HashBytes];
    md5.getHash(digest);
    std::vector<unsigned char> digestVector(digest, digest + sizeof(digest));
    return Base64::encode(digestVector);
	}

	bool uploadSegment(const std::filesystem::path &segmentPath,
			   const std::string &destinationURL) const
	try {
		using namespace curlpp::options;

		curlpp::Cleanup cleaner;
		curlpp::Easy request;

		request.setOpt(new Verbose(true));
		request.setOpt(new Url(destinationURL));
		request.setOpt(new Put(true));

		std::ifstream ifs(segmentPath, std::ios::binary);
		const auto fileSize = std::filesystem::file_size(segmentPath);
		request.setOpt(new ReadStream(&ifs));
		request.setOpt(new InfileSize(fileSize));
		request.setOpt(new Upload(true));

		std::list<std::string> headers{
			"Content-Length: " + std::to_string(fileSize),
			"Content-Type: audio/opus",
      "Content-MD5: " + calculateMD5Hash(segmentPath),
		};
		request.setOpt(new HttpHeader(headers));

		std::ostringstream oss;
		request.setOpt(new WriteStream(&oss));

		request.perform();

		obs_log(LOG_INFO, "%s", oss.str().c_str());

		return true;
	} catch (curlpp::LogicError &e) {
		obs_log(LOG_WARNING, e.what());
		return false;
	} catch (curlpp::RuntimeError &e) {
		obs_log(LOG_WARNING, e.what());
		return false;
	} catch (nlohmann::json::exception &e) {
		obs_log(LOG_WARNING, e.what());
		return false;
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
		if (segmentEntries.empty()) {
			return true;
		}

		if (destinationURLs.empty()) {
			loadNewDestinations(0, destinationBatchCount);
		}

		for (size_t i = 0; i < segmentEntries.size() - 1; i++) {
			const SegmentEntry &segmentEntry = segmentEntries[i];
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

			uploadSegment(segmentPath, uploadURL);

			if (index + destinationBatchBackoff >= count) {
				loadNewDestinations(count,
						    destinationBatchCount);
			}
		}

		segmentEntries.erase(segmentEntries.begin(),
				     segmentEntries.end() - 1);

		return true;
	}

	void write(const float *pcm, int samplesPerChannel)
	{
		ope_encoder_write_float(encoder, pcm, samplesPerChannel);
	}
};
