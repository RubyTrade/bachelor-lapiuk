#ifndef ACCOUNT_REST_API_CONTROLLER_HPP
#define ACCOUNT_REST_API_CONTROLLER_HPP

#include "core/controllers/account_rest_api_utils.hpp"
#include "core/net/net.hpp"
#include "core/parsers/account_rest_api_parser.hpp"
#include "core/utils/queue.hpp"

#include <memory>
#include <string>

namespace AccountRestApi {

class AccountRestApiController {
public:
  AccountRestApiController();

  bool request_account_info();
  bool request_account_balance();
  bool request_commission_rate(const std::string &symbol);

  bool pop_parsed_data(ParsedAccountRestApi &out);

  void register_parsed_data_callback(const std::function<void(const ParsedAccountRestApi &)> &callback) {
    m_parsedApiData->register_callback(callback);
  }

private:
  std::string _create_signed_query_string(const std::string &params);
  std::string _get_timestamp();
  bool _make_signed_request(const std::string &endpoint,
                            const std::string &params,
                            ACCOUNT_API_TYPE apiType);

private:
  std::unique_ptr<HttpSessionManager> m_http_session_manager;

  std::unique_ptr<ObservableQueue<ParsedAccountRestApi>> m_parsedApiData;
};

} // namespace AccountRestApi

#endif // ACCOUNT_REST_API_CONTROLLER_HPP
