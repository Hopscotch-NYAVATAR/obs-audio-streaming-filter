#include <chrono>

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
		true,
		json["customToken"].get<std::string>(),
		json["refreshTokenEndpoint"].get<std::string>(),
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

	request.setOpt(new Url(signInWithCustomTokenEndpoint));

	nlohmann::json payload{
		{"token", customToken},
		{"returnSecureToken", true},
	};
	std::string payloadString = payload.dump();
	request.setOpt(new PostFields(payloadString));
	request.setOpt(
		new PostFieldSize(static_cast<long>(payloadString.size())));

	std::list<std::string> headers{
		"Content-Type: application/json",
	};
	request.setOpt(new HttpHeader(headers));

	std::stringstream sstream;
	request.setOpt(new WriteStream(&sstream));

	request.perform();

	nlohmann::json json;
	sstream >> json;

	return {
		true,
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

RefreshIdTokenReponse
AuthClient::refreshIdToken(const std::string &_refreshTokenEndpoint,
			   const std::string _refreshToken) const
try {
	using namespace curlpp::options;
	using curlpp::FormParts::Content;

	curlpp::Cleanup cleaner;
	curlpp::Easy request;

	request.setOpt(new Url(_refreshTokenEndpoint));

	std::string formData("grant_type=refresh_token&refresh_token=" +
			     _refreshToken);
	request.setOpt(new PostFields(formData));

	std::stringstream sstream;
	request.setOpt(new WriteStream(&sstream));

	request.perform();

	nlohmann::json json;
	sstream >> json;

	return {
		true,
		json["expires_in"].get<std::string>(),
		json["token_type"].get<std::string>(),
		json["refresh_token"].get<std::string>(),
		json["id_token"].get<std::string>(),
		json["user_id"].get<std::string>(),
		json["project_id"].get<std::string>(),
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

static uint64_t getCurrentEpoch()
{
	using namespace std::chrono;
	const system_clock::time_point now = system_clock::now();
	const seconds s = duration_cast<seconds>(now.time_since_epoch());
	return s.count();
}

bool AuthClient::authenticateWithIndefiniteAccessToken(
	const std::string &indefiniteAccessTokenExchangeEndpoint,
	const std::string &indefiniteAccessToken)
{
	auto fetchResponse = fetchCustomToken(
		indefiniteAccessTokenExchangeEndpoint, indefiniteAccessToken);
	refreshTokenEndpoint = fetchResponse.refreshTokenEndpoint;

	if (!fetchResponse.success) {
		return false;
	}

	auto exchangeResponse = exchangeCustomToken(
		fetchResponse.customToken,
		fetchResponse.signInWithCustomTokenEndpoint);

	if (!exchangeResponse.success) {
		return false;
	}

	idToken = exchangeResponse.idToken;
	refreshToken = exchangeResponse.refreshToken;
	const uint64_t now = getCurrentEpoch();
	const uint64_t expiresIn = std::stoull(exchangeResponse.expiresIn);
	expiresAt = now + expiresIn;

	return true;
}

bool AuthClient::refresh(void)
{
	auto refreshResponse =
		refreshIdToken(refreshTokenEndpoint, refreshToken);
	if (!refreshResponse.success) {
		return false;
	}

	const uint64_t now = getCurrentEpoch();
	const uint64_t expiresIn = std::stoull(refreshResponse.expiresIn);
	expiresAt = now + expiresIn;
	refreshToken = refreshResponse.refreshToken;
	idToken = refreshResponse.idToken;

	return true;
}

std::string AuthClient::getIdToken(void)
{
	const uint64_t now = getCurrentEpoch();
	if (now + refreshBackoff >= expiresAt) {
		if (!refresh()) {
			obs_log(LOG_ERROR, "ID token refresh failed!");
			return "";
		}
	}
	return idToken;
}
