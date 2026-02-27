#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <sstream>
#include <streambuf>
#include <string>
#include <thread>
#include <variant>

#include "core/account_manager/account_controller.hpp"
#include "core/account_manager/order_book.hpp"
#include "core/controllers/market_data_controller.hpp"
#include "core/controllers/trading_stream_controller.hpp"
#include "core/controllers/user_data_stream_controller.hpp"
#include "core/net/net.hpp"
#include "core/parsers/market_data_parser.hpp"
#include "core/stream/market_stream.hpp"
#include "core/stream/trading_stream.hpp"
#include "core/stream/user_data_stream.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/dotenv.hpp"
#include "core/utils/fixed_num.hpp"
#include "core/utils/json.hpp"
#include "core/utils/log.hpp"
#include "core/utils/thread.hpp"
#include "core/utils/time.hpp"

void test_user_data_parsing() {
  using namespace UserData;

  struct TestCase {
    std::string jsonStr;
    std::string
        expectedType; // "TradeLite", "OrderTradeUpdate", "AccountUpdate"
  };

  std::vector<TestCase> testCases = {

      // --- TRADE_LITE
      {R"({"e":"TRADE_LITE","E":1771856732584,"T":1771856732584,"s":"SOLUSDT","q":"0.10","p":"0.0000","m":false,"c":"SOLUSDT_BOTH_1771856732181_1","S":"BUY","L":"80.1900","l":"0.10","t":3181138052,"i":199033476993})",
       "TradeLite"},

      // --- ORDER_TRADE_UPDATE
      {R"({"e":"ORDER_TRADE_UPDATE","T":1771856732584,"E":1771856732584,"o":{"s":"SOLUSDT","c":"SOLUSDT_BOTH_1771856732181_1","S":"BUY","o":"MARKET","f":"GTC","q":"0.1","p":"0","ap":"0","sp":"0","x":"NEW","X":"NEW","i":199033476993,"l":"0","z":"0","L":"0","n":"0","N":"USDT","T":1771856732584,"t":0,"b":"0","a":"0","m":false,"R":false,"wt":"CONTRACT_PRICE","ot":"MARKET","ps":"BOTH","cp":false,"rp":"0","pP":false,"si":0,"ss":0,"V":"EXPIRE_MAKER","pm":"NONE","gtd":0,"er":"0"}})",
       "OrderTradeUpdate"},

      // --- ACCOUNT_UPDATE
      {R"({"e":"ACCOUNT_UPDATE","T":1771856732584,"E":1771856732584,"a":{"B":[{"a":"USDT","wb":"1.73744723","cw":"1.73744723","bc":"0"}],"P":[{"s":"SOLUSDT","pa":"0.1","ep":"80.19","cr":"26.83159999","up":"0","mt":"cross","iw":"0","ps":"BOTH","ma":"USDT","bep":"80.230095"}],"m":"ORDER"}})",
       "AccountUpdate"}};

  for (auto &tc : testCases) {

    JSONQuery jsonMsg(tc.jsonStr);

    auto typeVal = jsonMsg.get_value(std::string(MsgKeys::EVENT_TYPE));

    std::string typeStr =
        (typeVal && typeVal->is_string() ? typeVal->get<std::string>() : "");

    if (typeStr.empty())
      return;

    USER_DATA_EVENT_TYPE eventType =
        str_to_type(USER_DATA_EVENT_TYPE_STR, typeStr);

    StreamMessage msg{eventType, jsonMsg};

    ParsedUserData parsedMsg = UserDataStreamParser::parse(msg);

    std::cout << "Testing event: " << typeStr << " -> ";

    if (tc.expectedType == "TradeLite" &&
        std::holds_alternative<TradeLiteEvent>(parsedMsg))
      std::cout << "PASS\n";

    else if (tc.expectedType == "OrderTradeUpdate" &&
             std::holds_alternative<OrderTradeUpdateEvent>(parsedMsg))
      std::cout << "PASS\n";

    else if (tc.expectedType == "AccountUpdate" &&
             std::holds_alternative<AccountUpdateEvent>(parsedMsg))
      std::cout << "PASS\n";

    else
      std::cout << "FAIL\n";
  }

  // --- ERROR TESTS
  std::vector<std::string> errorTests = {

      // TRADE_LITE broken
      R"({"e":"TRADE_LITE","s":"SOLUSDT"})",

      // ORDER_TRADE_UPDATE without "o"
      R"({"e":"ORDER_TRADE_UPDATE"})",

      // ACCOUNT_UPDATE without "a"
      R"({"e":"ACCOUNT_UPDATE"})"};

  for (auto &js : errorTests) {
    JSONQuery jsonMsg(js);

    auto typeVal = jsonMsg.get_value(std::string(MsgKeys::EVENT_TYPE));

    std::string typeStr =
        (typeVal && typeVal->is_string() ? typeVal->get<std::string>() : "");

    if (typeStr.empty())
      return;

    USER_DATA_EVENT_TYPE eventType =
        str_to_type(USER_DATA_EVENT_TYPE_STR, typeStr);

    StreamMessage msg{eventType, jsonMsg};

    ParsedUserData parsedMsg = UserDataStreamParser::parse(msg);

    if (std::holds_alternative<ErrorParse>(parsedMsg))
      std::cout << "Error test PASS\n";
    else
      std::cout << "Error test FAIL\n";
  }
}

void test_market_data_parsing() {
  using namespace Market;

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

  std::vector<std::string> errorTests = {
      R"({"stream":"btcusdt@trade","data":{"E":1771263510064}})",
      R"({"stream":"btcusdt@markPrice@1s","data":{"E":1771263510000,"s":"BTCUSDT"}})"};

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

  // MarketStream test

  // test_market_data_parsing();
  // test_user_data_parsing();

  /*
  std::unique_ptr<Market::MarketDataController> market_controller =
      std::make_unique<Market::MarketDataController>();

  Market::MarketRequest req1{"btcusdt", MARKET_DATA_TYPE::AGG_TRADE};
  market_controller->subscribe_to(req1);

  std::this_thread::sleep_for(std::chrono::seconds(5));

  Market::MarketRequest req2{"btcusdt", MARKET_DATA_TYPE::AGG_TRADE};
  market_controller->unsubscribe_from(req2);

  std::this_thread::sleep_for(std::chrono::seconds(2));

  for (auto &elem : market_controller->get_list_of_subscriptions()) {
    Log::log(elem.symbol);
  }
  */
  // UserDataStream test
  std::unique_ptr<UserData::UserDataStreamController> userdata_controller =
      std::make_unique<UserData::UserDataStreamController>();

  std::unique_ptr<OrderBook> order_book = std::make_unique<OrderBook>();
  std::unique_ptr<AccountController> account_controller =
      std::make_unique<AccountController>();

  userdata_controller->subscribe_to_publisher(order_book.get());
  userdata_controller->subscribe_to_publisher(account_controller.get());

  /*
  // Fixed num test
  Fixed num(123.1232434, 2);
  Fixed num2(0.0015555555, 6);

  Log::log("Fixed num: ", false);
  Log::log((num2 + num2).to_string());
  Log::log((num > num2) ? "true" : "false");
*/
  // TradingStream test
  std::unique_ptr<Trading::TradingStreamController> trading_controller =
      std::make_unique<Trading::TradingStreamController>();

  Trading::TradeRequest req(ORDER_SIDE::BUY, POSITION_SIDE::BOTH,
                            ORDER_TYPE::MARKET, "SOLUSDT",
                            Fixed::str_to_fixed("0.10"));

  trading_controller->create_order(req);
  std::this_thread::sleep_for(std::chrono::seconds(2));
  trading_controller->get_order_status(req);

  std::this_thread::sleep_for(std::chrono::seconds(3));

  Trading::TradeRequest req2(ORDER_SIDE::SELL, POSITION_SIDE::BOTH,
                             ORDER_TYPE::MARKET, "SOLUSDT",
                             Fixed::str_to_fixed("0.10"));

  req2.setReduceOnly(true);
  trading_controller->create_order(req2);

  std::optional<OrderEntry> order =
      order_book->getOrderByClientId(req.getClientOrderId());

  std::this_thread::sleep_for(std::chrono::seconds(2));

  for (auto &asset : account_controller->getBalancesList()) {
    Log::log("Asset: " + asset);
  }

  for (auto &pos : account_controller->getPositionsList()) {
    Log::log("Position: " + pos);
  }

  std::optional<OrderEntry> order2 =
      order_book->getOrderByClientId(req2.getClientOrderId());

  Log::log("order2: " + order2->symbol);

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
