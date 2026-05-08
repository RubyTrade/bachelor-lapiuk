#include "account_rest_api_controller.hpp"
#include "core/controllers/account_rest_api_utils.hpp"
#include "core/parsers/account_rest_api_parser.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/crypto.hpp"
#include "core/utils/dotenv.hpp"
#include "core/utils/helper_utils.hpp"
#include "core/utils/log.hpp"
#include "core/utils/queue.hpp"

#include <chrono>
#include <variant>

using namespace AccountRestApi;

AccountRestApiController::AccountRestApiController()
    : m_parsedApiData(
          std::make_unique<ObservableQueue<ParsedAccountRestApi>>()),
      m_http_session_manager(Net::init_http_session_manager()) {}

bool AccountRestApiController::pop_parsed_data(ParsedAccountRestApi &out) {
  return m_parsedApiData->pop_message(out);
}

std::string AccountRestApiController::_get_timestamp() {
  auto now = std::chrono::system_clock::now();
  uint64_t timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              now.time_since_epoch())
                              .count();
  return std::to_string(timestamp_ms);
}

std::string AccountRestApiController::_create_signed_query_string(
    const std::string &params) {
  std::string timestamp = _get_timestamp();
  std::string query_string = params.empty() ? "" : params + "&";
  query_string += "timestamp=" + timestamp;

  // Get API secret from environment
  std::string apiSecret = Env::getInstance().getenv(
      std::string(Data::BINANCE_READ_REST_SECRET_KEY_ENV));

  if (apiSecret.empty()) {
    Log::log_err("BINANCE_PRIVATE_KEY environment variable is not set");
    return {};
  }

  // Sign the query string with HMAC-SHA256
  std::string signature = Crypto::sign_hmac_sha256(apiSecret, query_string);

  if (signature.empty()) {
    Log::log_err("Failed to sign request");
    return {};
  }

  query_string += "&signature=" + signature;

  return query_string;
}

bool AccountRestApiController::_make_signed_request(const std::string &endpoint,
                                                    const std::string &params,
                                                    ACCOUNT_API_TYPE apiType) {

  std::string signed_query = _create_signed_query_string(params);

  if (signed_query.empty()) {
    Log::log_err("Failed to create signed query string");
    return false;
  }

  std::string target = endpoint + "?" + signed_query;
  std::string buffer{};

  // Get API key from environment
  std::string apiKey = Env::getInstance().getenv(
      std::string(Data::BINANCE_READ_REST_APIKEY_ENV));

  if (apiKey.empty()) {
    Log::log_err(
        "BINANCE_READ_REST_APIKEY_ENV environment variable is not set");
    return false;
  }

  // Build HTTP query
  std::optional<HttpQuery> query =
      HttpQueryBuilder(HttpMethod::GET)
          .setHost(Data::API_HOST)
          .setPort(Data::HTTPS_PORT)
          .setTarget(target)
          .setHeaders({{std::string(Data::Header::APIKEY), apiKey}})
          .commit();

  if (!query.has_value()) {
    Log::log_err("Failed to build HTTP query");
    return false;
  }

  // Make the HTTP request
  NetError err = m_http_session_manager->do_request(query.value(), buffer);

  if (err.hasError()) {
    Log::log_err("HTTP request failed: " + err.getMessage());
    return false;
  }

  if (buffer.empty()) {
    Log::log_err("Empty response from server");
    return false;
  }

  Log::log_debug("REST_API: " + buffer);

  // Parse the response
  JSONQuery jsonResponse(buffer);
  RestApiMessage restMsg(apiType, jsonResponse);

  ParsedAccountRestApi parsedData = AccountRestApiParser::parse(restMsg);

  // Check if parsing was successful
  if (std::holds_alternative<ErrorParse>(parsedData)) {
    auto error = std::get<ErrorParse>(parsedData);
    return false;
  }

  // Store in queue
  m_parsedApiData->push_message(std::move(parsedData));

  return true;
}

bool AccountRestApiController::request_account_info() {
  return _make_signed_request(std::string(Data::ApiEndpoints::ACCOUNT_INFO), "",
                              ACCOUNT_API_TYPE::ACCOUNT_INFO);
}

bool AccountRestApiController::request_account_balance() {
  return _make_signed_request(std::string(Data::ApiEndpoints::ACCOUNT_BALANCE),
                              "", ACCOUNT_API_TYPE::ACCOUNT_BALANCE);
}

bool AccountRestApiController::request_commission_rate(
    const std::string &symbol) {
  if (symbol.empty()) {
    Log::log_err("Symbol cannot be empty for commission rate request");
    return false;
  }

  std::string params = "symbol=" + symbol;
  return _make_signed_request(std::string(Data::ApiEndpoints::COMMISSION_RATE),
                              params, ACCOUNT_API_TYPE::COMMISSION_RATE);
}
