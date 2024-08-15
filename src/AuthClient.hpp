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

static FetchCustomTokenResponse fetchCustomToken(std::string indefiniteAccessTokenExchangeEndpoint,
            std::string indefiniteAccessToken)
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

struct ExchangeCustomTokenResponse {
  const std::string idToken;
  const std::string refreshToken;
  const std::string expiresIn;
};

static ExchangeCustomTokenResponse
exchangeCustomToken(const std::string &customToken,
        const std::string &signInWithCustomTokenEndpoint)
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
  request.setOpt(new PostFieldSize(payloadString.size()));
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
