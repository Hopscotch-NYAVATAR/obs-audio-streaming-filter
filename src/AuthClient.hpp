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
	const std::string customToken;
	const std::string refreshTokenEndpoint;
	const std::string signInWithCustomTokenEndpoint;
};

struct ExchangeCustomTokenResponse {
	const bool success;
	const std::string idToken;
	const std::string refreshToken;
	const std::string expiresIn;
};

struct RefreshIdTokenReponse {
	const bool success;
	const std::string expiresIn;
	const std::string tokenType;
	const std::string refreshToken;
	const std::string idToken;
	const std::string userId;
	const std::string projectID;
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

	bool authenticateWithIndefiniteAccessToken(
		const std::string &indefiniteAccessTokenExchangeEndpoint,
		const std::string &indefiniteAccessToken);

	std::string getIdToken(void);
};
