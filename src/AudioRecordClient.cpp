#include "AudioRecordClient.hpp"

#include <sstream>
#include <list>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <nlohmann/json.hpp>

#include <obs.h>

#include "plugin-support.h"

BatchGetAudioRecordUploadDestinationResponse
AudioRecordClient::batchGetUploadDestination(const std::string &idToken,
					     int start, int count,
					     const std::string &prefix) const
try {
	using namespace curlpp::options;

	curlpp::Cleanup cleaner;
	curlpp::Easy request;

	request.setOpt(new Url(batchIssueUploadDestinationEndpoiint));

	std::list<std::string> headers{
		"Authorization: Bearer " + idToken,
	};
	request.setOpt(new HttpHeader(headers));

	std::ostringstream formDataStream;
	formDataStream << "start=" << start << "&count=" << count
		       << "&prefix=" << prefix;
	std::string formData(formDataStream.str());
	request.setOpt(new PostFields(formData));

	std::stringstream sstream;
	request.setOpt(new WriteStream(&sstream));

	request.perform();

	nlohmann::json json;
	sstream >> json;

	return {
		true,
		json["destinations"].template get<std::vector<std::string>>(),
	};
} catch (curlpp::LogicError &e) {
	obs_log(LOG_WARNING, e.what());
	return {};
} catch (curlpp::RuntimeError &e) {
	obs_log(LOG_WARNING, e.what());
	return {};
} catch (nlohmann::json::exception &e) {
	obs_log(LOG_WARNING, e.what());
	return {};
}
