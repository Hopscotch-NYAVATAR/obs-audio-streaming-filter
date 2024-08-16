#pragma once

#include <filesystem>
#include <thread>
#include <atomic>
#include <mutex>

#include <opusenc.h>

#include "AudioRecordClient.hpp"
#include "AuthClient.hpp"

struct SegmentEntry {
	int segmentIndex;
	std::filesystem::path segmentPath;
};

class OpusUploaderThreadFunctor {
	AuthClient &authClient;
	const AudioRecordClient &audioRecordClient;

	const std::filesystem::path &outputDirectory;
	const std::string &outputExt;
	const std::string &outputPrefix;

	std::mutex segmentEntriesMutex;
	std::vector<SegmentEntry> segmentEntries;

	std::vector<std::string> destinationURLs;

public:
	int destinationBatchCount = 200;
	int destinationBatchBackoff = 100;

  std::atomic_bool isStopping;

	OpusUploaderThreadFunctor(AuthClient &authClient,
				  const AudioRecordClient &audioRecordClient,
				  const std::filesystem::path &outputDirectory,
				  const std::string &outputExt,
				  const std::string &outputPrefix);
	void operator()(void);
	void addSegmentEntry(const SegmentEntry &segmentEntry);

private:
	void uploadPendingSegments(void);
	void loadNewDestinations(int start, int count);
};

class OpusUploader {
	const std::filesystem::path outputDirectory;
	const std::string outputExt;
	const std::string outputPrefix;

	OggOpusComments *comments;
	OggOpusEnc *encoder;
	int segmentIndex;
	SegmentEntry ongoingSegment;

	OpusUploaderThreadFunctor uploadFunctor;

public:
	OpusUploader(const std::filesystem::path &outputDirectory,
		     const std::string &outputExt,
		     const std::string &outputPrefix, int sampleRate,
		     const AudioRecordClient &audioRecordClient,
		     AuthClient &authClient);
	~OpusUploader(void);
	bool continueNewStream(void);
	void uploadPendingSegments(void);
	void write(const float *pcmBuffer, int samplesPerChannel);
};
