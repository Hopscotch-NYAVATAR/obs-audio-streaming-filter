#pragma once

#include <sstream>
#include <string>
#include <cstdint>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <nlohmann/json.hpp>

#include <obs.h>

#include "plugin-support.h"

struct FetchCustomTokenResponse {
	const bool success;
	const std::string batchIssueAudioRecordUploadDestinationEndpoint;
	const std::string customToken;
	const std::string refreshTokenEndpoint;
	const std::string signInWithCustomTokenEndpoint;
};

struct ExchangeCustomTokenResponse {
	bool success;
	std::string idToken;
	std::string refreshToken;
	std::string expiresIn;
};

struct RefreshIdTokenReponse {
	bool success;
	std::string expiresIn;
	std::string tokenType;
	std::string refreshToken;
	std::string idToken;
	std::string userId;
	std::string projectID;
};

struct AuthenticatAuthClienteWithIndefiniteAccessTokenResult {
	bool success;
	std::string batchIssueAudioRecordUploadDestinationEndpoint;
};

class AuthClient {
	std::string refreshTokenEndpoint;
	std::string idToken;
	std::string refreshToken;
	uint64_t expiresAt;

public:
	uint64_t refreshBackoff = 300;

	FetchCustomTokenResponse fetchCustomToken(
		const std::string &indefiniteAccessTokenExchangeEndpoint,
		const std::string &indefiniteAccessToken) const;

	ExchangeCustomTokenResponse exchangeCustomToken(
		const std::string &customToken,
		const std::string &signInWithCustomTokenEndpoint) const;

	RefreshIdTokenReponse
	refreshIdToken(const std::string &_refreshTokenEndpoint,
		       const std::string _refreshToken) const;

	bool refresh(void);

	AuthenticatAuthClienteWithIndefiniteAccessTokenResult
	authenticateWithIndefiniteAccessToken(
		const std::string &indefiniteAccessTokenExchangeEndpoint,
		const std::string &indefiniteAccessToken);

	std::string getIdToken(void);
};
