#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <streambuf>
#include <string>
#include <thread>
#include <variant>

#include "core/controllers/market_data_controller.hpp"
#include "core/net/net.hpp"
#include "core/parsers/market_data_parser.hpp"
#include "core/stream/market_stream.hpp"
#include "core/stream/trading_stream.hpp"
#include "core/stream/user_data_stream.hpp"
#include "core/utils/dotenv.hpp"
#include "core/utils/fixed_num.hpp"
#include "core/utils/json.hpp"
#include "core/utils/log.hpp"
#include "core/utils/thread.hpp"
#include "core/utils/time.hpp"

void test_market_data_parsing() {
  struct TestCase {
    std::string jsonStr;
    std::string expectedType; // "TradeData", "AggTradeData", etc.
  };

  std::vector<TestCase> testCases = {
      {R"({"stream":"btcusdt@markPrice@1s","data":{"e":"markPriceUpdate","E":1771263510000,"s":"BTCUSDT","p":"68005.80802899","P":"67836.82194783","i":"68035.14043478","r":"-0.00004435","T":1771286400000}})",
       "MarkPriceData"},
      {R"({"stream":"btcusdt@trade","data":{"e":"trade","E":1771263510064,"T":1771263510064,"s":"BTCUSDT","t":7297376207,"p":"68005.00","q":"0.100","X":"MARKET","m":true}})",
       "TradeData"},
      {R"({"stream":"btcusdt@aggTrade","data":{"e":"aggTrade","E":1771263510195,"a":3141966230,"s":"BTCUSDT","p":"68005.00","q":"2.695","nq":"2.695","f":7297376181,"l":7297376207,"T":1771263510041,"m":true}})",
       "AggTradeData"},
      {R"({"stream":"btcusdt@bookTicker","data":{"e":"bookTicker","u":9924360161244,"s":"BTCUSDT","b":"68005.00","B":"2.394","a":"68005.10","A":"0.857","T":1771263510337,"E":1771263510337}})",
       "BookTickerData"},
      {R"({"stream":"btcusdt@depth5@100ms","data":{"e":"depthUpdate","E":1771263510140,"T":1771263510139,"s":"BTCUSDT","U":9924360134447,"u":9924360146998,"pu":9924360134267,"b":[["68005.00","2.214"],["68004.90","0.042"]],"a":[["68005.10","0.838"],["68005.20","0.218"]]}})",
       "DepthData"}};

  for (auto &tc : testCases) {
    JSONQuery jsonMsg(tc.jsonStr);
    auto keyVal = jsonMsg.get_value(std::string(MsgKeys::STREAM));
    auto value = jsonMsg.get_value(std::string(MsgKeys::STREAM_DATA));

    std::string keyStr =
        (keyVal && keyVal->is_string() ? keyVal->get<std::string>()
                                       : std::string(MsgKeys::DEFAULT_VAL));
    JSONQuery jsonData =
        (value && value->is_object() ? JSONQuery{value.value()} : JSONQuery{});

    StreamMessage msg{keyStr, jsonData};

    ParsedMarketData parsedMsg = MarketDataParser::parse(msg);

    std::cout << "Testing stream: " << keyStr << " -> ";
    if (tc.expectedType == "TradeData" &&
        std::holds_alternative<TradeData>(parsedMsg))
      std::cout << "PASS\n";
    else if (tc.expectedType == "AggTradeData" &&
             std::holds_alternative<AggTradeData>(parsedMsg))
      std::cout << "PASS\n";
    else if (tc.expectedType == "MarkPriceData" &&
             std::holds_alternative<MarkPriceData>(parsedMsg))
      std::cout << "PASS\n";
    else if (tc.expectedType == "BookTickerData" &&
             std::holds_alternative<BookTickerData>(parsedMsg))
      std::cout << "PASS\n";
    else if (tc.expectedType == "DepthData" &&
             std::holds_alternative<DepthData>(parsedMsg))
      std::cout << "PASS\n";
    else
      std::cout << "FAIL\n";
  }

  // Тести на помилку
  std::vector<std::string> errorTests = {
      R"({"stream":"btcusdt@trade","data":{"E":1771263510064}})", // немає
                                                                  // symbol,
                                                                  // tradeId,
                                                                  // price
      R"({"stream":"btcusdt@markPrice@1s","data":{"E":1771263510000,"s":"BTCUSDT"}})" // немає markPrice
  };

  for (auto &js : errorTests) {
    JSONQuery jsonMsg(js);
    auto keyVal = jsonMsg.get_value(std::string(MsgKeys::STREAM));
    auto value = jsonMsg.get_value(std::string(MsgKeys::STREAM_DATA));

    std::string keyStr =
        (keyVal && keyVal->is_string() ? keyVal->get<std::string>()
                                       : std::string(MsgKeys::DEFAULT_VAL));
    JSONQuery jsonData =
        (value && value->is_object() ? JSONQuery{value.value()} : JSONQuery{});

    StreamMessage msg{keyStr, jsonData};
    ParsedMarketData parsedMsg = MarketDataParser::parse(msg);

    if (std::holds_alternative<ErrorParse>(parsedMsg))
      std::cout << "Error test PASS\n";
    else
      std::cout << "Error test FAIL\n";
  }
}

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

  // MarketStream test

  // test_market_data_parsing();
  /*
    std::unique_ptr<MarketDataController> market_controller =
        std::make_unique<MarketDataController>();

    MarketRequest req1{"btcusdt", MARKET_DATA_TYPE::AGG_TRADE};
    market_controller->subscribe_to(req1);
  */
  // UserDataStream test
  /*
    Queue<std::string> msgQueueUser;
    std::unique_ptr<UserDataStream> ustream =
        std::make_unique<UserDataStream>(msgQueueUser);
    ustream->connect_to_websocket();

    Thread thread11;
    Thread thread22;
    thread11.start(&UserDataStream::start_listening, ustream.get());

    thread22
        .start([&msgQueueUser]() {
          while (true) {
            std::string out_msg;
            bool res = msgQueueUser.pop_message(out_msg);
            if (res) {
              Log::log(out_msg);
            }
          }
        })
        .detach();
        */
  /*
  // Fixed num test
  Fixed num(123.1232434, 2);
  Fixed num2(0.0015555555, 6);

  Log::log("Fixed num: ", false);
  Log::log((num2 + num2).to_string());
  Log::log((num > num2) ? "true" : "false");
*/
  // TradingStream test
  /*
  Queue<std::string> msgQueue;
  std::unique_ptr<TradingStream> stream =
      std::make_unique<TradingStream>(msgQueue);
  stream->connect_to_websocket();

  Thread thread1;
  Thread thread2;
  thread1.start(&TradingStream::start_listening, stream.get()).detach();

  thread2
      .start([&msgQueue]() {
        while (true) {
          std::string out_msg;
          bool res = msgQueue.pop_message(out_msg);
          if (res) {
            Log::log(out_msg);
          }
        }
      })
      .detach();
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
    std::optional<JSONQuery> param_query =
        ParametersBuilder()
            .add_side(ORDER_SIDE::BUY)
            .add_positionSide(POSITION_SIDE::BOTH)
            .add_quantity(Fixed::str_to_fixed("0.11"))
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
