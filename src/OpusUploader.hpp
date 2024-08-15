#pragma once

#include <filesystem>
#include <sstream>
#include <iomanip>
#include <iostream>

#include <opusenc.h>

#include "RecordPathGenerator.hpp"

class OpusUploader {
	const std::filesystem::path outputDirectory;
	const std::string outputExt;
	const std::string outputPrefix;

	OggOpusComments *comments;
	OggOpusEnc *encoder;

	int segmentIndex;

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

public:
	OpusUploader(std::filesystem::path _outputDirectory,
		     std::string _outputExt, std::string _outputPrefix,
		     int sampleRate)
		: outputDirectory(_outputDirectory),
		  outputExt(_outputExt),
		  outputPrefix(_outputPrefix)
	{
		using std::filesystem::path;

		comments = ope_comments_create();

		path outputPath(generateNextStreamPath(outputDirectory,
						       outputExt, outputPrefix,
						       segmentIndex));

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
		std::filesystem::create_directories(outputPath.parent_path());

		int error = ope_encoder_continue_new_file(
			encoder, outputPath.string().c_str(), comments);
		if (error) {
			obs_log(LOG_ERROR, "%s", ope_strerror(error));
			return false;
		}
		return true;
	}

	void write(const float *pcm, int samplesPerChannel)
	{
		ope_encoder_write_float(encoder, pcm, samplesPerChannel);
	}
};
