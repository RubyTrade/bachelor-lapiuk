#include <memory>
#include <string>

#include "net/net.hpp"
#include "stream/market_stream.hpp"
#include "utils/log.hpp"
#include "utils/thread.hpp"

int main(int argc, char *argv[]) {
  std::unique_ptr<MarketStream> stream = std::make_unique<MarketStream>();
  WSError wsErr = stream->connect_to_websocket();
  std::optional<JSONQuery> query =
      StreamQueryBuilder(MARKET_STREAM_METHOD::SUBSCRIBE)
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

  return EXIT_SUCCESS;
}
