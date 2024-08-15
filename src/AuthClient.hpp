#pragma once

#include <sstream>
#include <string>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <nlohmann/json.hpp>

#include <obs.h>

#include "plugin-support.h"

struct FetchCustomTokenResponse {
	const std::string customToken;
	const std::string signInWithCustomTokenEndpoint;
};

struct ExchangeCustomTokenResponse {
	const std::string idToken;
	const std::string refreshToken;
	const std::string expiresIn;
};

class AuthClient {
public:
	FetchCustomTokenResponse fetchCustomToken(
		const std::string &indefiniteAccessTokenExchangeEndpoint,
		const std::string &indefiniteAccessToken) const;

	ExchangeCustomTokenResponse exchangeCustomToken(
		const std::string &customToken,
		const std::string &signInWithCustomTokenEndpoint) const;
};
