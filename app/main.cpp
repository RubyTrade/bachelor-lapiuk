#include <boost/beast/websocket/detail/frame.hpp>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <sstream>
#include <streambuf>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "core/account_manager/account_controller.hpp"
#include "core/account_manager/order_book.hpp"
#include "core/controllers/account_rest_api_controller.hpp"
#include "core/controllers/market_data_controller.hpp"
#include "core/controllers/trading_stream_controller.hpp"
#include "core/controllers/user_data_stream_controller.hpp"
#include "core/net/net.hpp"
#include "core/parsers/account_rest_api_parser.hpp"
#include "core/parsers/common_parser_utils.hpp"
#include "core/parsers/market_data_parser.hpp"
#include "core/parsers/trading_stream_parser.hpp"
#include "core/stream/market_stream.hpp"
#include "core/stream/trading_stream.hpp"
#include "core/stream/user_data_stream.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/dotenv.hpp"
#include "core/utils/fixed_num.hpp"
#include "core/utils/helper_utils.hpp"
#include "core/utils/json.hpp"
#include "core/utils/log.hpp"
#include "core/utils/thread.hpp"
#include "core/utils/time.hpp"
#include "strategy/strategies/data_dispatcher.hpp"
#include "strategy/strategies/mean_reversion.hpp"
#include "strategy/strategies/strategy_manager.hpp"

int main(int argc, char *argv[]) {
  std::string dotenv_path = ".env";
  std::string pk_path = "binance_private.pem";
  if (argc > 1) {
    dotenv_path = argv[1];
    pk_path = argv[2];
  }

  std::ifstream file(pk_path, std::ios::binary);
  std::ostringstream oss;
  if (file) {
    oss << file.rdbuf();
  }

  Env::getInstance().setenv("BINANCE_PRIVATE_KEY", oss.str());
  Log::set_log_file_from_env();

  /*
  std::unique_ptr<AccountRestApi::AccountRestApiController> controller =
      std::make_unique<AccountRestApi::AccountRestApiController>();

  controller->register_parsed_data_callback(
      [&](const AccountRestApi::ParsedAccountRestApi &data) {
        if (!std::holds_alternative<ErrorParse>(data)) {
          Log::log_debug("success");
        }
        return;
      });

  controller->request_account_info();
  controller->request_account_balance();
  controller->request_commission_rate("BTCUSDT");
*/
  // MarketStream test
  std::unique_ptr<Market::MarketDataController> market_controller =
      std::make_unique<Market::MarketDataController>();

  Market::MarketRequest req1{"solusdt", MARKET_DATA_TYPE::AGG_TRADE};
  market_controller->subscribe_to(req1);

  std::this_thread::sleep_for(std::chrono::seconds(2));

  // UserDataStream test
  std::unique_ptr<UserData::UserDataStreamController> userdata_controller =
      std::make_unique<UserData::UserDataStreamController>();

  std::this_thread::sleep_for(std::chrono::seconds(2));
  std::unique_ptr<OrderBook> order_book = std::make_unique<OrderBook>();
  std::unique_ptr<AccountController> account_controller =
      std::make_unique<AccountController>();

  userdata_controller->subscribe_to_publisher(order_book.get());
  userdata_controller->subscribe_to_publisher(account_controller.get());
  // TradingStream test
  std::unique_ptr<Trading::TradingStreamController> trading_controller =
      std::make_unique<Trading::TradingStreamController>();

  // Strategy test
  std::unique_ptr<StrategyManager> strat_manager =
      std::make_unique<StrategyManager>(
          trading_controller.get(), order_book.get(), account_controller.get());

  market_controller->subscribe_to_publisher(
      strat_manager->getMarketDispatcher());

  StrategyInfo mean_info =
      strat_manager->create_strategy(STRATEGIES::MEAN_REVERSION);

  SIGNAL signal = SIGNAL::LONG_ENTRY;
  if (signal != SIGNAL::HOLD) {
    Log::log_info("\nSignal of " + mean_info.strategy_str + " | " +
                  type_to_str(SIGNAL_STR, signal));
    strat_manager->process_signal(mean_info, signal);
  }

  std::this_thread::sleep_for(std::chrono::seconds(5));

  // strat_manager->run_loop(mean_info);

  /*
  std::this_thread::sleep_for(std::chrono::seconds(15));
  signal = SIGNAL::CLOSE_LONG;
  if (signal != SIGNAL::HOLD) {
    Log::log_info("\nSignal of " + mean_info.strategy_str + " | " +
                  type_to_str(SIGNAL_STR, signal));
    strat_manager->process_signal(mean_info, signal);
  }
  */

  /*
  std::optional<JSONQuery> balance_info =
      TradingStreamQueryBuilder(USER_DATA_STREAM_METHOD::ACCOUNT_BALANCE)
          .commit();

  if (balance_info) {
    stream->execute_query(balance_info.value());
  }

  std::optional<JSONQuery> param_query_2 =
      ParametersBuilder().add_symbol("SOLUSDT").commit();

  std::optional<JSONQuery> pos_info =
      TradingStreamQueryBuilder(USER_DATA_STREAM_METHOD::ACCOUNT_POSITION)
          .add_borderless_params(param_query_2.value())
          .commit();

  if (pos_info) {
    stream->execute_query(pos_info.value());
  }
  */
  /*
  std::optional<JSONQuery> sub_query =
      TradingStreamQueryBuilder(USER_DATA_STREAM_METHOD::STREAM_SUBSCRIBE)
          .commit();

  if (sub_query) {
    std::ostringstream ss;
    ss << "\nQuery: " << sub_query.value().str();
    Log::log_debug(ss.str());

    stream->execute_query(sub_query.value());
  }

  std::optional<JSONQuery> test_order =
      TradingStreamQueryBuilder(USER_DATA_STREAM_METHOD::ORDER_PLACE)
                   .commit();

  if (test_order) {
    std::ostringstream ss;
    ss << "\nQuery: " << test_order.value().str();
    Log::log_debug(ss.str());

    stream->execute_query(test_order.value());
  }
  */
  /*
  std::unique_ptr<HttpRequest> client = Net::init_http_client();

  std::string buffer;

  std::string binance_key =
  Env::getInstance().getenv("BINANCE_READ_API_KEY");

  client->do_request(HttpMethod::POST, "api.binance.com", 443,
                     "/api/v3/userDataStream", "", true, buffer,
                     {{"X-MBX-APIKEY", binance_key}});

  std::ostringstream ss;
  ss << "\nHttp response: " << buffer;
  Log::log_debug(ss.str());
  */
  /*
  AsyncTimer timer;
  std::condition_variable cv;
  std::mutex mtx;
  std::unique_lock<std::mutex> lock(mtx);
  bool flag = false;
  timer.start(std::chrono::seconds(10), cv, mtx, flag);

  cv.wait(lock, [&flag] { return flag; });
  Log::log_debug("funny thing");
  timer.start(std::chrono::seconds(5), []() { Log::log_debug("Testing"); });
  */

  std::cin.get();
  return EXIT_SUCCESS;
}
