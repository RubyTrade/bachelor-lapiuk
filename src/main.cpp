#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <streambuf>
#include <string>
#include <thread>

#include "core/net/net.hpp"
#include "core/stream/market_stream.hpp"
#include "core/stream/trading_stream.hpp"
#include "core/stream/user_data_stream.hpp"
#include "core/utils/dotenv.hpp"
#include "core/utils/fixed_num.hpp"
#include "core/utils/json.hpp"
#include "core/utils/log.hpp"
#include "core/utils/thread.hpp"
#include "core/utils/time.hpp"

int main(int argc, char *argv[]) {
  std::string dotenv_path = ".env";
  std::string pk_path = "private.pem";
  if (argc > 1) {
    dotenv_path = argv[1];
    pk_path = argv[2];

    std::ifstream file(pk_path, std::ios::binary);
    std::ostringstream oss;
    if (file) {
      oss << file.rdbuf();
    }

    Env::getInstance().setenv("BINANCE_PRIVATE_KEY", oss.str());
  }

  /*
  // MarketStream test
  std::unique_ptr<MarketStream> stream = std::make_unique<MarketStream>();
  NetError wsErr = stream->connect_to_websocket();
  std::optional<JSONQuery> query =
      MarketStreamQueryBuilder(MARKET_STREAM_METHOD::SUBSCRIBE)
          .add_trade_symbol("dogeusdt")
          .commit();

  if (query.has_value()) {
    std::ostringstream ss;
    ss << "\nQuery: " << query.value().str();
    Log::log(ss.str());

    stream->execute_query(query.value());
  }

  Thread thread1;
  Thread thread2;
  if (!wsErr.hasError())
    thread1.start(&MarketStream::start_listening, stream.get());

  thread2.start(&MarketStream::start_reading, stream.get());
  */
  /*
  // UserDataStream test
  std::unique_ptr<UserDataStream> ustream = std::make_unique<UserDataStream>();
  ustream->connect_to_websocket();

  Thread thread11;
  Thread thread22;
  thread11.start(&UserDataStream::start_listening, ustream.get());

  thread22.start(&UserDataStream::start_reading, ustream.get());
*/

  /*
  // Fixed num test
  Fixed num(123.1232434, 2);
  Fixed num2(50.1445, 4);

  Log::log("Fixed num: ", false);
  Log::log((num2 - num).to_string());
  Log::log((num > num2) ? "true" : "false");
  */

  /*
  // TradingStream test
  std::unique_ptr<TradingStream> stream = std::make_unique<TradingStream>();
  stream->connect_to_websocket();

  Thread thread1;
  Thread thread2;
  thread1.start(&TradingStream::start_listening, stream.get()).detach();

  thread2.start(&TradingStream::start_reading, stream.get()).detach();

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
  std::optional<JSONQuery> param_query =
      ParametersBuilder()
          .add_side(ORDER_SIDE::BUY)
          .add_positionSide(POSITION_SIDE::BOTH)
          .add_quantity(0.11)
          .add_symbol("SOLUSDT")
          .add_type(ORDER_TYPE::MARKET)
          .commit();

  std::optional<JSONQuery> subscribe_query =
      TradingStreamQueryBuilder(USER_DATA_STREAM_METHOD::ORDER_PLACE)
          .add_borderless_params(param_query.value())
          .commit();

  if (subscribe_query) {
    stream->execute_query(subscribe_query.value());
  }
*/

  /*
  std::optional<JSONQuery> sub_query =
      TradingStreamQueryBuilder(USER_DATA_STREAM_METHOD::STREAM_SUBSCRIBE)
          .commit();

  if (sub_query) {
    std::ostringstream ss;
    ss << "\nQuery: " << sub_query.value().str();
    Log::log(ss.str());

    stream->execute_query(sub_query.value());
  }

  std::optional<JSONQuery> test_order =
      TradingStreamQueryBuilder(USER_DATA_STREAM_METHOD::ORDER_PLACE)
                   .commit();

  if (test_order) {
    std::ostringstream ss;
    ss << "\nQuery: " << test_order.value().str();
    Log::log(ss.str());

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
  Log::log(ss.str());
*/
  /*
  AsyncTimer timer;
  std::condition_variable cv;
  std::mutex mtx;
  std::unique_lock<std::mutex> lock(mtx);
  bool flag = false;
  timer.start(std::chrono::seconds(10), cv, mtx, flag);

  cv.wait(lock, [&flag] { return flag; });
  Log::log("funny thing");
  timer.start(std::chrono::seconds(5), []() { Log::log("Testing"); });
  */

  std::cin.get();
  return EXIT_SUCCESS;
}
