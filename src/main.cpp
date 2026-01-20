#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <sstream>
#include <streambuf>
#include <string>
#include <thread>

#include "net/net.hpp"
#include "stream/market_stream.hpp"
#include "stream/user_data_stream.hpp"
#include "utils/dotenv.hpp"
#include "utils/log.hpp"
#include "utils/thread.hpp"
#include "utils/time.hpp"

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

    Env::getInstance().setenv("BINANCE_READ_PRIVATE_KEY", oss.str());
  }

  // MarketStream MarketStream test
  /*
    std::unique_ptr<MarketStream> stream = std::make_unique<MarketStream>();
    NetError wsErr = stream->connect_to_websocket();
    std::optional<JSONQuery> query =
        MarketStreamQueryBuilder(MARKET_STREAM_METHOD::SUBSCRIBE)
            .add_aggTrade_symbol("pepeusdt")
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
  // UserDataStream test
  std::unique_ptr<UserDataStream> stream = std::make_unique<UserDataStream>();
  stream->connect_to_websocket();

  std::string binance_key = Env::getInstance().getenv("BINANCE_READ_API_KEY");

  std::optional<JSONQuery> auth_query =
      UserDataStreamQueryBuilder(USER_DATA_STREAM_METHOD::SESSION_LOGON)
          .add_apiKey(binance_key)
          .commit();

  if (auth_query)
    stream->execute_query(auth_query.value());

  std::optional<JSONQuery> sub_query =
      UserDataStreamQueryBuilder(USER_DATA_STREAM_METHOD::STREAM_SUBSCRIBE)
          .commit();

  if (sub_query)
    stream->execute_query(sub_query.value());

  Thread thread1;
  Thread thread2;
  thread1.start(&MarketStream::start_listening, stream.get());

  thread2.start(&MarketStream::start_reading, stream.get());
  /*
  std::unique_ptr<HttpRequest> client = Net::init_http_client();

  std::string buffer;

  std::string binance_key = Env::getInstance().getenv("BINANCE_READ_API_KEY");

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
