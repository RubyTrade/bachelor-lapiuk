#include "user_data_stream_controller.hpp"

#include "core/parsers/common_parser_utils.hpp"
#include "core/parsers/user_data_stream_parser.hpp"
#include "core/stream/user_data_stream.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/dotenv.hpp"
#include "core/utils/helper_utils.hpp"
#include "core/utils/json.hpp"
#include "core/utils/log.hpp"
#include "core/utils/queue.hpp"

#include <utility>
#include <variant>

using namespace UserData;

UserDataStreamController::UserDataStreamController()
    : m_listenThread(std::make_unique<Thread>()),
      m_readThread(std::make_unique<Thread>()),
      m_userDataMsgQueues(std::make_unique<MessageStreams>()),
      m_parsedStreamData(std::make_unique<ObservableQueue<ParsedUserData>>()),
      m_http_session_manager(Net::init_http_session_manager()),
      m_key_timer(std::make_unique<AsyncTimer>()),
      m_eventPublisher(std::make_unique<EventPublisher<ParsedUserData>>()) {

  m_userDataStream =
      std::make_unique<UserDataStream>(m_userDataMsgQueues->msgQueue);

  std::string listenKey = _create_listenKey();
  if (listenKey.empty()) {
    Log::log_err(
        "User data: empty listenKey — check BINANCE_READ_API_KEY and POST "
        "/fapi/v1/listenKey");
  }

  m_key_timer->start_recurring(LISTEN_KEY_EXPIRY_TIME,
                               [this]() { this->_update_listenKey(); });

  NetError wsErr = m_userDataStream->connect_to_websocket(listenKey);
  if (!wsErr.hasError()) {
    m_is_stream_running = true;
    _start_listen_thread();
    _start_read_thread();
  } else {
    Log::log_err("User data WebSocket connect failed (private/ws path)");
  }

  m_userDataMsgQueues->streamQueue.register_callback(
      [this](const StreamMessage &msg) {
        if (msg.userDataType == USER_DATA_EVENT_TYPE::LISTEN_KEY_EXPIRY) {
          _reconnect();
        } else {
          m_parsedStreamData->push_message(
              std::move(UserDataStreamParser::parse(msg)));
        }
      });

  m_parsedStreamData->register_callback(
      [this](const ParsedUserData &parsedMsg) {
        if (!std::holds_alternative<ErrorParse>(parsedMsg)) {
          m_eventPublisher->publish(parsedMsg);
        }
      });
}

void UserDataStreamController::subscribe_to_publisher(
    IEventListener<ParsedUserData> *listener) {
  m_eventPublisher->subscribe(listener);
}

void UserDataStreamController::unsubscribe_from_publisher(
    IEventListener<ParsedUserData> *listener) {
  m_eventPublisher->unsubscribe(listener);
}

void UserDataStreamController::_reconnect() {
  m_is_stream_running = false;

  m_listenThread->stop();

  m_userDataMsgQueues->msgQueue.stop_queue();

  m_readThread->stop();
  m_userDataStream->disconnect_from_websocket();

  std::string listenKey = _create_listenKey();

  m_key_timer->stop();
  m_key_timer->start_recurring(LISTEN_KEY_EXPIRY_TIME,
                               [this]() { this->_update_listenKey(); });

  NetError wsErr = m_userDataStream->connect_to_websocket(listenKey);
  if (!wsErr.hasError()) {
    m_userDataMsgQueues->msgQueue.start_queue();
    m_is_stream_running = true;
    _start_listen_thread();
    _start_read_thread();
  }
}

std::string UserDataStreamController::_create_listenKey() {
  std::string listenKey{};
  std::string buffer{};
  std::string apiKey =
      Env::getInstance().getenv(std::string(Data::BINANCE_READ_APIKEY_ENV));

  std::optional<HttpQuery> query =
      HttpQueryBuilder(HttpMethod::POST)
          .setHost(Data::API_HOST)
          .setPort(Data::HTTPS_PORT)
          .setTarget(std::string(Data::API_DEFAULT_TARGET) +
                     std::string(LISTEN_KEY_STR))
          .setHeaders({{std::string(Data::Header::APIKEY), apiKey}})
          .commit();

  if (query.has_value()) {
    m_http_session_manager->do_request(query.value(), buffer);
  }

  if (buffer.empty())
    return {};

  std::optional<nlohmann::json> json_val =
      JSONQuery(buffer).get_value(std::string(LISTEN_KEY_STR));

  if (json_val.has_value() && json_val.value().is_string()) {
    listenKey = json_val.value().get_ref<const std::string &>();
  }

  return listenKey;
}

void UserDataStreamController::_update_listenKey() {
  std::string buffer{};
  std::string apiKey =
      Env::getInstance().getenv(std::string(Data::BINANCE_READ_APIKEY_ENV));

  std::optional<HttpQuery> query =
      HttpQueryBuilder(HttpMethod::PUT)
          .setHost(Data::API_HOST)
          .setPort(Data::HTTPS_PORT)
          .setTarget(std::string(Data::API_DEFAULT_TARGET) +
                     std::string(LISTEN_KEY_STR))
          .setHeaders({{std::string(Data::Header::APIKEY), apiKey}})
          .commit();

  if (query.has_value()) {
    m_http_session_manager->do_request(query.value(), buffer);
  }
}

void UserDataStreamController::_start_listen_thread() {
  auto errHandler = [this]() { _reconnect(); };

  m_listenThread->start(
      [ptr = m_userDataStream.get(), h = std::move(errHandler)]() mutable {
        ptr->start_listening(std::move(h));
      });
}

void UserDataStreamController::_start_read_thread() {
  m_readThread->start(&UserDataStreamController::_start_buffer_reading, this);
}

void UserDataStreamController::_start_buffer_reading() {
  while (m_is_stream_running) {
    std::string out_msg;
    bool res = m_userDataMsgQueues->msgQueue.pop_message(out_msg);
    if (res) {
      Log::log_debug("USERDATA: " + out_msg);
      _parse_msg(std::move(out_msg));
    }
  }
}

void UserDataStreamController::_parse_msg(const std::string &&msg) {
  JSONQuery jsonMsg(msg);

  auto typeVal = jsonMsg.get_value(std::string(MsgKeys::EVENT_TYPE));

  std::string typeStr =
      (typeVal && typeVal->is_string() ? typeVal->get<std::string>() : "");

  if (typeStr.empty())
    return;

  USER_DATA_EVENT_TYPE eventType =
      str_to_type(USER_DATA_EVENT_TYPE_STR, typeStr);

  StreamMessage res{eventType, jsonMsg};

  m_userDataMsgQueues->streamQueue.push_message(std::move(res));
}
