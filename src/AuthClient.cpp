#include "AuthClient.hpp"

FetchCustomTokenResponse AuthClient::fetchCustomToken(
	const std::string &indefiniteAccessTokenExchangeEndpoint,
	const std::string &indefiniteAccessToken) const
try {
	using namespace curlpp::options;

	curlpp::Cleanup cleaner;
	curlpp::Easy request;

	request.setOpt(new Url(indefiniteAccessTokenExchangeEndpoint));
	request.setOpt(new PostFields(""));
	request.setOpt(new PostFieldSize(0));

	std::list<std::string> headers{"Authorization: Bearer " +
				       indefiniteAccessToken};
	request.setOpt(new HttpHeader(headers));

	std::stringstream sstream;
	request.setOpt(new WriteStream(&sstream));

	request.perform();

	nlohmann::json json;
	sstream >> json;

	return {
		json["customToken"].get<std::string>(),
		json["signInWithCustomTokenEndpoint"].get<std::string>(),
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

ExchangeCustomTokenResponse AuthClient::exchangeCustomToken(
	const std::string &customToken,
	const std::string &signInWithCustomTokenEndpoint) const
try {
	using namespace curlpp::options;
	using curlpp::FormParts::Content;

	curlpp::Cleanup cleaner;
	curlpp::Easy request;

	request.setOpt(new Verbose(true));

	request.setOpt(new Url(signInWithCustomTokenEndpoint));

	nlohmann::json payload{
		{"token", customToken},
		{"returnSecureToken", true},
	};
	std::string payloadString = payload.dump();
	request.setOpt(new PostFields(payloadString));
	request.setOpt(new PostFieldSize(static_cast<long>(payloadString.size())));
	obs_log(LOG_INFO, "%s", payloadString.c_str());
	obs_log(LOG_INFO, "%s", signInWithCustomTokenEndpoint.c_str());

	std::list<std::string> headers{
		"Content-Type: application/json",
	};
	request.setOpt(new HttpHeader(headers));

	std::stringstream sstream;
	request.setOpt(new WriteStream(&sstream));

	request.perform();

	nlohmann::json json;
	sstream >> json;
	obs_log(LOG_INFO, "%s", sstream.str().c_str());

	return {
		json["idToken"].get<std::string>(),
		json["refreshToken"].get<std::string>(),
		json["expiresIn"].get<std::string>(),
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
