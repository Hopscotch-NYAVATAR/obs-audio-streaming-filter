#pragma once

#include <string>
#include <vector>

struct BatchGetAudioRecordUploadDestinationResponse {
	bool success;
	std::vector<std::string> destinations;
};

struct GetAudioRecordDestinationsResult {
	bool success;
	std::string ext;
	std::string prefix;
	int start;
	int count;
	std::vector<std::string> destinations;
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
