/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat_client.h"

#include <algorithm>
#include <sstream>
#include <map>

#include "ledger_impl.h"
#include "bat_helper.h"
#include "rapidjson_bat_helper.h"
#include "static_values.h"

#include "wally_bip39.h"
#include "anon/anon.h"

using namespace std::placeholders;

namespace braveledger_bat_client {

BatClient::BatClient(bat_ledger::LedgerImpl* ledger) :
      ledger_(ledger) {
  initAnonize();
}

BatClient::~BatClient() {
}

void BatClient::registerPersona(const std::string& safetynet_token) {
  if (!safetynet_token.empty()) {
    std::string safetynet_prefix = PREFIX_V5;
#if defined (OS_ANDROID) && defined(ARCH_CPU_X86_FAMILY) && defined(OFFICIAL_BUILD)
    safetynet_prefix = PREFIX_V3;
#endif
    std::vector<std::string> headers;
    headers.push_back("safetynet-token:" + safetynet_token);
    auto callback = std::bind(&BatClient::registerPersonaSafetyNetCallback, this, _1, _2, _3);
    ledger_->LoadURL(braveledger_bat_helper::buildURL(
          (std::string)GET_SET_PROMOTION, safetynet_prefix),
        headers, "", "", ledger::URL_METHOD::GET, callback);
    return;
  }
  auto callback = std::bind(&BatClient::requestCredentialsCallback, this,
      _1, _2, _3);
  ledger_->LoadURL(
      braveledger_bat_helper::buildURL(REGISTER_PERSONA, PREFIX_V2),
      std::vector<std::string>(), "", "",
      ledger::URL_METHOD::GET, callback);
}

void BatClient::registerPersonaSafetyNetCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {

  ledger_->LogResponse(__func__, response_status_code, response, headers);

  unsigned int statusCode;
  std::string error;
  bool hasResponseError = braveledger_bat_helper::getJSONResponse(
    response, statusCode, error);
  if (hasResponseError && statusCode == 404) {
    // TODO(samartnik): as per discussion with @evq we decided to use this
    // quick hack, because we are going to move on new API soon,
    // where SafetyNet check failure will be explicit
    std::string message;
    if (braveledger_bat_helper::getJSONMessage(response, message) &&
      message == SAFETYNET_ERROR_MESSAGE) {
      ledger_->OnWalletInitialized(ledger::Result::SAFETYNET_ATTESTATION_FAILED);
      return;
    }
  }
  // We passed safetynet check, so just make regular call
  registerPersona("");
}

void BatClient::requestCredentialsCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != 200) {
    ledger_->OnWalletInitialized(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }

  std::string persona_id = ledger_->GetPersonaId();

  if (persona_id.empty()) {
    persona_id = ledger_->GenerateGUID();
    ledger_->SetPersonaId(persona_id);
  }
  // Anonize2 limit is 31 octets
  std::string user_id = persona_id;
  user_id.erase(
      std::remove(user_id.begin(), user_id.end(), '-'), user_id.end());
  user_id.erase(12, 1);

  ledger_->SetUserId(user_id);

  std::string registrar_vk = ledger_->GetRegistrarVK();
  if (!braveledger_bat_helper::getJSONValue(REGISTRARVK_FIELDNAME,
                                            response,
                                            registrar_vk)) {
    ledger_->OnWalletInitialized(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }
  DCHECK(!registrar_vk.empty());
  ledger_->SetRegistrarVK(registrar_vk);
  std::string pre_flight;
  std::string proof = getAnonizeProof(registrar_vk, user_id, pre_flight);
  ledger_->SetPreFlight(pre_flight);

  if (proof.empty()) {
    ledger_->OnWalletInitialized(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }

  braveledger_bat_helper::WALLET_INFO_ST wallet_info = ledger_->GetWalletInfo();
  std::vector<uint8_t> key_info_seed = braveledger_bat_helper::generateSeed();

  wallet_info.keyInfoSeed_ = key_info_seed;
  ledger_->SetWalletInfo(wallet_info);
  std::vector<uint8_t> secretKey =
      braveledger_bat_helper::getHKDF(key_info_seed);
  std::vector<uint8_t> publicKey;
  std::vector<uint8_t> newSecretKey;
  braveledger_bat_helper::getPublicKeyFromSeed(secretKey,
                                               publicKey,
                                               newSecretKey);
  std::string label = ledger_->GenerateGUID();
  std::string publicKeyHex = braveledger_bat_helper::uint8ToHex(publicKey);
  std::string keys[3] = {"currency", "label", "publicKey"};
  std::string values[3] = {CURRENCY, label, publicKeyHex};
  std::string octets = braveledger_bat_helper::stringify(keys, values, 3);
  std::string headerDigest = "SHA-256=" +
      braveledger_bat_helper::getBase64(
          braveledger_bat_helper::getSHA256(octets));
  std::string headerKeys[1] = {"digest"};
  std::string headerValues[1] = {headerDigest};
  std::string headerSignature = braveledger_bat_helper::sign(headerKeys,
                                                             headerValues,
                                                             1,
                                                             "primary",
                                                             newSecretKey);

  braveledger_bat_helper::REQUEST_CREDENTIALS_ST requestCredentials;
  requestCredentials.requestType_ = "httpSignature";
  requestCredentials.proof_ = proof;
  requestCredentials.request_body_currency_ = CURRENCY;
  requestCredentials.request_body_label_ = label;
  requestCredentials.request_body_publicKey_ = publicKeyHex;
  requestCredentials.request_headers_digest_ = headerDigest;
  requestCredentials.request_headers_signature_ = headerSignature;
  requestCredentials.request_body_octets_ = octets;
  std::string payloadStringify =
      braveledger_bat_helper::stringifyRequestCredentialsSt(requestCredentials);
  std::vector<std::string> registerHeaders;
  registerHeaders.push_back("Content-Type: application/json; charset=UTF-8");

  // We should use simple callbacks on iOS
  const std::string url = braveledger_bat_helper::buildURL(
      (std::string)REGISTER_PERSONA + "/" + ledger_->GetUserId(), PREFIX_V2);
  auto callback = std::bind(&BatClient::registerPersonaCallback, this,
      _1, _2, _3);
  ledger_->LoadURL(
    url,
    registerHeaders, payloadStringify, "application/json; charset=utf-8",
    ledger::URL_METHOD::POST, callback);
}

std::string BatClient::getAnonizeProof(const std::string& registrarVK,
                                       const std::string& id,
                                       std::string& preFlight) {
  const char* cred = makeCred(id.c_str());
  if (nullptr != cred) {
    preFlight = cred;
    free((void*)cred);
  } else {
    return "";
  }
  const char* proofTemp = registerUserMessage(preFlight.c_str(),
                                              registrarVK.c_str());
  std::string proof;
  if (nullptr != proofTemp) {
    proof = proofTemp;
    free((void*)proofTemp);
  } else {
    return "";
  }

  return proof;
}

void BatClient::registerPersonaCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != 200) {
    ledger_->OnWalletInitialized(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }

  std::string verification;
  if (!braveledger_bat_helper::getJSONValue(VERIFICATION_FIELDNAME,
                                            response,
                                            verification)) {
    ledger_->OnWalletInitialized(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }
  const char* masterUserToken = registerUserFinal(
      ledger_->GetUserId().c_str(),
      verification.c_str(),
      ledger_->GetPreFlight().c_str(),
      ledger_->GetRegistrarVK().c_str());

  if (nullptr != masterUserToken) {
    ledger_->SetMasterUserToken(masterUserToken);
    free((void*)masterUserToken);
  } else if (!braveledger_bat_helper::ignore_for_testing()) {
    ledger_->OnWalletInitialized(
        ledger::Result::REGISTRATION_VERIFICATION_FAILED);
    return;
  }

  braveledger_bat_helper::WALLET_INFO_ST wallet_info = ledger_->GetWalletInfo();
  unsigned int days;
  double fee_amount = .0;
  std::string currency;
  if (!braveledger_bat_helper::getJSONWalletInfo(response,
                                                 wallet_info,
                                                 currency,
                                                 fee_amount,
                                                 days)) {
    ledger_->OnWalletInitialized(ledger::Result::BAD_REGISTRATION_RESPONSE);
    return;
  }

  ledger_->SetWalletInfo(wallet_info);
  ledger_->SetCurrency(currency);
  ledger_->SetContributionAmount(fee_amount);
  ledger_->SetDays(days);
  ledger_->SetBootStamp(braveledger_bat_helper::currentTime());
  ledger_->ResetReconcileStamp();
  ledger_->OnWalletInitialized(ledger::Result::WALLET_CREATED);
}

void BatClient::getWalletProperties() {
  std::string payment_id = ledger_->GetPaymentId();
  std::string passphrase = ledger_->GetWalletPassphrase();

  if (payment_id.empty() || passphrase.empty()) {
    braveledger_bat_helper::WALLET_PROPERTIES_ST properties;
    ledger_->OnWalletProperties(ledger::Result::CORRUPTED_WALLET, properties);
    return;
  }

  std::string path = (std::string)WALLET_PROPERTIES
      + payment_id
      + WALLET_PROPERTIES_END;
  const std::string url = braveledger_bat_helper::buildURL(
      path,
      PREFIX_V2,
      braveledger_bat_helper::SERVER_TYPES::BALANCE);
  auto callback = std::bind(&BatClient::walletPropertiesCallback, this,
      _1, _2, _3);
  ledger_->LoadURL(url,
                   std::vector<std::string>(),
                   std::string(),
                   std::string(),
                   ledger::URL_METHOD::GET,
                   callback);
}

void BatClient::walletPropertiesCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  braveledger_bat_helper::WALLET_PROPERTIES_ST properties;
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  if (response_status_code != 200) {
   ledger_->OnWalletProperties(ledger::Result::LEDGER_ERROR, properties);
   return;
  }

  bool ok = braveledger_bat_helper::loadFromJson(properties, response);
  if (!ok) {
   BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
     "Failed to load wallet properties state";
   ledger_->OnWalletProperties(ledger::Result::LEDGER_ERROR, properties);
   return;
  }

  ledger_->SetWalletProperties(properties);
  ledger_->OnWalletProperties(ledger::Result::LEDGER_OK, properties);
}

std::string BatClient::getWalletPassphrase() const {
  braveledger_bat_helper::WALLET_INFO_ST wallet_info = ledger_->GetWalletInfo();
  DCHECK(wallet_info.keyInfoSeed_.size());
  std::string passPhrase;
  if (0 == wallet_info.keyInfoSeed_.size()) {
    return passPhrase;
  }
    char* words = nullptr;
  int result = bip39_mnemonic_from_bytes(nullptr,
                                         &wallet_info.keyInfoSeed_.front(),
                                         wallet_info.keyInfoSeed_.size(),
                                         &words);
  if (0 != result) {
    DCHECK(false);

    return passPhrase;
  }
  passPhrase = words;
  wally_free_string(words);

  return passPhrase;
}

void BatClient::recoverWallet(const std::string& pass_phrase) {
  size_t written = 0;
  std::vector<unsigned char> newSeed;
  if (braveledger_bat_helper::split(pass_phrase,
    WALLET_PASSPHRASE_DELIM).size() == 16) {
    // use niceware for legacy wallet passphrases
    ledger_->LoadNicewareList(
      std::bind(&BatClient::OnNicewareListLoaded, this, pass_phrase, _1, _2));
  } else {
    std::vector<unsigned char> newSeed;
    newSeed.resize(32);
    int result = bip39_mnemonic_to_bytes
      (nullptr, pass_phrase.c_str(), &newSeed.front(),
      newSeed.size(), &written);
    continueRecover(result, &written, newSeed);
  }
}

void BatClient::OnNicewareListLoaded(const std::string& pass_phrase,
                                     ledger::Result result,
                                     const std::string& data) {
  if (result == ledger::Result::LEDGER_OK &&
    braveledger_bat_helper::split(pass_phrase,
    WALLET_PASSPHRASE_DELIM).size() == 16) {
    std::vector<uint8_t> seed;
    seed.resize(32);
    size_t written = 0;
    uint8_t nwResult = braveledger_bat_helper::niceware_mnemonic_to_bytes(
      pass_phrase, seed, &written, braveledger_bat_helper::split(
      data, DICTIONARY_DELIMITER));
    continueRecover(nwResult, &written, seed);
  } else {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
      << "Failed to load niceware list";
    std::vector<braveledger_bat_helper::GRANT> empty;
    ledger_->OnRecoverWallet(result, 0, empty);
    return;
  }
}

void BatClient::continueRecover(int result,
                                size_t *written,
                                std::vector<uint8_t>& newSeed) {
  if (0 != result || 0 == *written) {
    BLOG(ledger_, ledger::LogLevel::LOG_INFO)
      << "Result: "
      << result
      << " Size: "
      << *written;
    std::vector<braveledger_bat_helper::GRANT> empty;
    ledger_->OnRecoverWallet(ledger::Result::LEDGER_ERROR, 0, empty);
    return;
  }


  braveledger_bat_helper::WALLET_INFO_ST wallet_info = ledger_->GetWalletInfo();
  wallet_info.keyInfoSeed_ = newSeed;
  ledger_->SetWalletInfo(wallet_info);

  std::vector<uint8_t> secretKey = braveledger_bat_helper::getHKDF(newSeed);
  std::vector<uint8_t> publicKey;
  std::vector<uint8_t> newSecretKey;
  braveledger_bat_helper::getPublicKeyFromSeed(secretKey,
                                               publicKey,
                                               newSecretKey);
  std::string publicKeyHex = braveledger_bat_helper::uint8ToHex(publicKey);

  auto callback = std::bind(&BatClient::recoverWalletPublicKeyCallback, this,
      _1, _2, _3);
  ledger_->LoadURL(braveledger_bat_helper::buildURL(
        (std::string)RECOVER_WALLET_PUBLIC_KEY + publicKeyHex, PREFIX_V2),
    std::vector<std::string>(), "", "",
    ledger::URL_METHOD::GET, callback);
}

void BatClient::recoverWalletPublicKeyCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != 200) {
    std::vector<braveledger_bat_helper::GRANT> empty;
    ledger_->OnRecoverWallet(ledger::Result::LEDGER_ERROR, 0, empty);
    return;
  }
  std::string recoveryId;
  braveledger_bat_helper::getJSONValue("paymentId", response, recoveryId);

  auto callback = std::bind(&BatClient::recoverWalletCallback, this,
        _1, _2, _3, recoveryId);
  ledger_->LoadURL(braveledger_bat_helper::buildURL(
        (std::string)WALLET_PROPERTIES + recoveryId, PREFIX_V2),
      std::vector<std::string>(), "", "", ledger::URL_METHOD::GET, callback);
}

void BatClient::recoverWalletCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const std::string& recoveryId) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  if (response_status_code != 200) {
    std::vector<braveledger_bat_helper::GRANT> empty;
    ledger_->OnRecoverWallet(ledger::Result::LEDGER_ERROR, 0, empty);
    return;
  }

  braveledger_bat_helper::WALLET_INFO_ST wallet_info = ledger_->GetWalletInfo();
  braveledger_bat_helper::WALLET_PROPERTIES_ST properties =
      ledger_->GetWalletProperties();
  unsigned int days;
  double fee_amount = .0;
  std::string currency;
  braveledger_bat_helper::getJSONWalletInfo(response,
                                            wallet_info,
                                            currency,
                                            fee_amount,
                                            days);
  braveledger_bat_helper::getJSONRecoverWallet(response,
                                               properties.balance_,
                                               properties.probi_,
                                               properties.grants_);
  ledger_->SetWalletInfo(wallet_info);
  ledger_->SetCurrency(currency);
  if (!ledger_->GetUserChangedContribution()) {
    ledger_->SetContributionAmount(fee_amount);
  }
  ledger_->SetDays(days);
  ledger_->SetWalletProperties(properties);
  ledger_->SetPaymentId(recoveryId);
  ledger_->OnRecoverWallet(ledger::Result::LEDGER_OK,
                           properties.balance_,
                           properties.grants_);
}

void BatClient::getGrants(const std::string& lang,
                          const std::string& forPaymentId,
                          const std::string& safetynet_token) {
  std::string paymentId = forPaymentId;
  if (paymentId.empty()) {
    paymentId = ledger_->GetPaymentId();
  }
  std::string arguments;
  if (!paymentId.empty() || !lang.empty()) {
    arguments = "?";
    if (!paymentId.empty()) {
      arguments += "paymentId=" + paymentId;
    }
    if (!lang.empty()) {
      if (arguments.length() > 1) {
        arguments += "&";
      }
      arguments += "lang=" + lang;
    }
  }

  std::vector<std::string> headers;
  if (!safetynet_token.empty()) {
    headers.push_back("safetynet-token:" + safetynet_token);
  }
  std::string safetynet_prefix = PREFIX_V5;
#if defined (OS_ANDROID) && defined(ARCH_CPU_X86_FAMILY) && defined(OFFICIAL_BUILD)
    safetynet_prefix = PREFIX_V3;
#endif
  auto callback = std::bind(&BatClient::getGrantsCallback, this, safetynet_token, _1, _2, _3);
  ledger_->LoadURL(braveledger_bat_helper::buildURL(
        (std::string)GET_SET_PROMOTION + arguments, safetynet_token.empty() ? PREFIX_V4 : safetynet_prefix),
      headers, "", "", ledger::URL_METHOD::GET, callback);
}

void BatClient::getGrantsCallback(
    std::string safetynet_token,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  braveledger_bat_helper::GRANT properties;
  braveledger_bat_helper::Grants grants;
  braveledger_bat_helper::GRANTS_PROPERTIES_ST grants_properties;

  ledger_->LogResponse(__func__, response_status_code, response, headers);

  unsigned int statusCode;
  std::string error;
  bool hasResponseError = braveledger_bat_helper::getJSONResponse(
    response, statusCode, error);
  if (hasResponseError && statusCode == 404) {
    ledger_->SetLastGrantLoadTimestamp(time(0));
    ledger_->OnGrant(ledger::Result::GRANT_NOT_FOUND, properties);
    return;
  }

  if (response_status_code != 200) {
    ledger_->OnGrant(ledger::Result::LEDGER_ERROR, properties);
    return;
  }

  bool ok = braveledger_bat_helper::loadFromJson(grants_properties, response);

  if (!ok && !safetynet_token.empty()) {
    ok = braveledger_bat_helper::loadFromJson(properties, response);
    if (ok) {
      braveledger_bat_helper::GRANT_RESPONSE grantResponse;
      grantResponse.promotionId = properties.promotionId;
      grantResponse.type = "ugp";
      grants_properties.grants_.push_back(grantResponse);
    }
  } 

  if (!ok) {
     BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
       "Failed to load grant properties state";
    ledger_->OnGrant(ledger::Result::LEDGER_ERROR, properties);
    return;
  }

  for (auto grant : grants_properties.grants_) {
    braveledger_bat_helper::GRANT grant_;
    grant_.promotionId = grant.promotionId;
    grant_.type = grant.type;

    grants.push_back(grant_);
    ledger_->OnGrant(ledger::Result::LEDGER_OK, grant_);
  }

  ledger_->SetLastGrantLoadTimestamp(time(0));
  ledger_->SetGrants(grants);
}

void BatClient::setGrant(const std::string& captchaResponse,
                         const std::string& promotionId,
                         const std::string& safetynet_token) {
  std::string promotionToSet = promotionId;
  if (promotionId.empty() && safetynet_token.empty()) {
    braveledger_bat_helper::GRANT properties;
    ledger_->OnGrantFinish(ledger::Result::LEDGER_ERROR, properties);
    return;
  }

  if (promotionId.empty() && !safetynet_token.empty()) {
    braveledger_bat_helper::Grants state_grants = ledger_->GetGrants();

    for (auto state_grant : state_grants) {
      if ("ugp" == state_grant.type) {
        promotionToSet = state_grant.promotionId;
        break;
      }
    }
    
  }

  std::string keys[2] = {"promotionId", "captchaResponse"};
  std::string values[2] = {promotionToSet, captchaResponse};
  std::string payload = braveledger_bat_helper::stringify(keys, values, safetynet_token.empty() ? 2 : 1);

  std::vector<std::string> headers;
  if (!safetynet_token.empty()) {
    headers.push_back("safetynet-token:" + safetynet_token);
  }

  auto callback = std::bind(&BatClient::setGrantCallback, this, _1, _2, _3, !safetynet_token.empty());
  ledger_->LoadURL(braveledger_bat_helper::buildURL(
        (std::string)GET_SET_PROMOTION + "/" + ledger_->GetPaymentId(),
        safetynet_token.empty() ? PREFIX_V2 : PREFIX_V3),
      headers, payload, "application/json; charset=utf-8",
      ledger::URL_METHOD::PUT, callback);
}

void BatClient::setGrantCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    bool is_safetynet_check) {
  std::string error;
  unsigned int statusCode;
  braveledger_bat_helper::GRANT grant;
  braveledger_bat_helper::getJSONResponse(response, statusCode, error);

  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (!error.empty()) {
    if (statusCode == 403) {
      ledger_->OnGrantFinish(is_safetynet_check ? ledger::Result::SAFETYNET_ATTESTATION_FAILED : ledger::Result::CAPTCHA_FAILED, grant);
    } else if (statusCode == 404 || statusCode == 410) {
      ledger_->OnGrantFinish(ledger::Result::GRANT_NOT_FOUND, grant);
    } else if (statusCode == 409) {
      ledger_->OnGrantFinish(ledger::Result::GRANT_ALREADY_CLAIMED, grant);
    } else {
      ledger_->OnGrantFinish(ledger::Result::LEDGER_ERROR, grant);
    }
    return;
  }

  bool ok = false;
  if (is_safetynet_check) {
    ok = grant.loadFromJsonSafetyNet(response);
  } else {
    ok = braveledger_bat_helper::loadFromJson(grant, response);
  }
  if (!ok) {
    ledger_->OnGrantFinish(ledger::Result::LEDGER_ERROR, grant);
    return;
  }

  braveledger_bat_helper::Grants updated_grants;
  braveledger_bat_helper::Grants state_grants = ledger_->GetGrants();

  for (auto state_grant : state_grants) {
    if (grant.type == state_grant.type) {
      grant.promotionId = state_grant.promotionId;
      ledger_->OnGrantFinish(ledger::Result::LEDGER_OK, grant);
      updated_grants.push_back(grant);
    } else {
      updated_grants.push_back(state_grant);
    }
  }

  ledger_->SetGrants(updated_grants);
}

void BatClient::getGrantCaptcha() {
  std::vector<std::string> headers;
  headers.push_back("brave-product:brave-core");
  auto callback = std::bind(&BatClient::getGrantCaptchaCallback, this,
      _1, _2, _3);
  ledger_->LoadURL(braveledger_bat_helper::buildURL(
        (std::string)GET_PROMOTION_CAPTCHA + ledger_->GetPaymentId(),
        PREFIX_V4),
      headers, "", "", ledger::URL_METHOD::GET, callback);
}

void BatClient::getGrantCaptchaCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  auto it = headers.find("captcha-hint");
  if (response_status_code != 200 || it == headers.end()) {
    // TODO NZ Add error handler
    return;
  }

  ledger_->OnGrantCaptcha(response, it->second);
}

void BatClient::GetAddressesForPaymentId(
    ledger::WalletAddressesCallback callback) {
  std::string currency = ledger_->GetCurrency();
  std::string path = (std::string)WALLET_PROPERTIES +
                      ledger_->GetPaymentId() +
                      "?refresh=true";
  ledger_->LoadURL(
      braveledger_bat_helper::buildURL(path, PREFIX_V2),
      std::vector<std::string>(),
      "",
      "",
      ledger::URL_METHOD::GET,
      std::bind(&BatClient::GetAddressesForPaymentIdCallback,
                this,
                _1,
                _2,
                _3,
                callback));
}

void BatClient::GetAddressesForPaymentIdCallback(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    ledger::WalletAddressesCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  std::map<std::string, std::string> addresses;
  bool ok = braveledger_bat_helper::getJSONAddresses(response, addresses);

  if (!ok) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
       "Failed to get addresses from payment ID";
    return;
  }

  callback(addresses);
  ledger_->SetAddresses(addresses);
}

void BatClient::getGrantViaSafetynetCheck() {
  ledger_->LoadURL(braveledger_bat_helper::buildURL(
      (std::string)GET_PROMOTION_ATTESTATION + ledger_->GetPaymentId(),
      PREFIX_V1),
  std::vector<std::string>(), "", "",
  ledger::URL_METHOD::GET, std::bind(&BatClient::getGrantViaSafetynetCheckCallback,
                                   this,
                                   _1,
                                   _2,
                                   _3));
}

void BatClient::getGrantViaSafetynetCheckCallback(int response_status_code,
                                                  const std::string& response,
                                                  const std::map<std::string,
                                                  std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != 200) {
    // Attestation failed
    braveledger_bat_helper::GRANT grant;
    ledger_->OnGrantFinish(ledger::Result::SAFETYNET_ATTESTATION_FAILED,
        grant);
    return;
  }
  std::string nonce;
  braveledger_bat_helper::getJSONValue("nonce", response, nonce);
  ledger_->OnGrantViaSafetynetCheck(nonce);
}
}  // namespace braveledger_bat_client