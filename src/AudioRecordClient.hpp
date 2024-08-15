#pragma once

#include <string>
#include <vector>

struct BatchGetAudioRecordUploadDestinationResponse {
	const bool success;
	const std::vector<std::string> destinations;
};

struct GetAudioRecordDestinationsResult {
	const bool success;
	const std::string ext;
	const std::string prefix;
	const int start;
	const int count;
	const std::vector<std::string> destinations;
};

class AudioRecordClient {
	std::string batchIssueUploadDestinationEndpoiint;

public:
	void
	init(const std::string &audioRecordUploadDestinationBatchIssueEndpoint)
	{
		batchIssueUploadDestinationEndpoiint =
			audioRecordUploadDestinationBatchIssueEndpoint;
	}

	BatchGetAudioRecordUploadDestinationResponse
	batchGetUploadDestination(const std::string &ext,
				  const std::string &prefix, int start,
				  int count, const std::string &idToken) const;

	GetAudioRecordDestinationsResult
	getDestinations(const std::string &ext, const std::string &prefix,
			int start, int count, const std::string &idToken) const;
};
