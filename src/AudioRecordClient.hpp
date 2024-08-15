#pragma once

#include <string>
#include <vector>

struct BatchGetAudioRecordUploadDestinationResponse {
	const bool success;
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
	batchGetUploadDestination(const std::string &idToken, int start,
				  int count, const std::string &prefix) const;
};
