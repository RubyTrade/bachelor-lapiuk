#include "user_data_stream_controller.hpp"
#include "core/controllers/user_data_stream_utils.hpp"
#include "core/parsers/user_data_stream_parser.hpp"
#include "core/stream/user_data_stream.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/dotenv.hpp"
#include "core/utils/helper_utils.hpp"
#include "core/utils/queue.hpp"
#include <list>

using namespace UserData;

// TODO: add listenKey validity check

UserDataStreamController::UserDataStreamController()
    : m_listenThread(std::make_unique<Thread>()),
      m_readThread(std::make_unique<Thread>()),
      m_userDataMsgQueues(std::make_unique<MessageStreams>()),
      m_parsedStreamData(std::make_unique<Queue<ParsedUserData>>()),
      m_http_session_manager(Net::init_http_session_manager()),
      m_key_timer(std::make_unique<AsyncTimer>()) {
  // User Data Stream init
  m_userDataStream =
      std::make_unique<UserDataStream>(m_userDataMsgQueues->msgQueue);

  std::string listenKey = _create_listenKey();

  m_key_timer->start_recurring(LISTEN_KEY_EXPIRY_TIME,
                               [this]() { this->_update_listenKey(); });

  NetError wsErr = m_userDataStream->connect_to_websocket(listenKey);
  if (!wsErr.hasError()) {
    _start_listen_thread();
    _start_read_thread();
  }

  m_userDataMsgQueues->streamQueue.register_callback(
      [this](const StreamMessage &msg) {
        // Overall, should not happen in normal flow, since
        // we update the key every 30 minutes
        if (msg.userDataType == USER_DATA_EVENT_TYPE::LISTEN_KEY_EXPIRY) {
          m_userDataStream->disconnect_from_websocket();
          std::string listenKey = _create_listenKey();

          m_key_timer->stop();
          m_key_timer->start_recurring(LISTEN_KEY_EXPIRY_TIME,
                                       [this]() { this->_update_listenKey(); });

          m_listenThread->stop();
          m_readThread->stop();

          NetError wsErr = m_userDataStream->connect_to_websocket(listenKey);
          if (!wsErr.hasError()) {
            _start_listen_thread();
            _start_read_thread();
          }

        } else {
          m_parsedStreamData->push_message(UserDataStreamParser::parse(msg));
        }
      });
};

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
  m_listenThread->start(&UserDataStream::start_listening,
                        m_userDataStream.get());
}
void UserDataStreamController::_start_read_thread() {
  m_readThread->start(&UserDataStreamController::_start_buffer_reading, this);
}

void UserDataStreamController::_start_buffer_reading() {
  while (true) {
    std::string out_msg;
    bool res = m_userDataMsgQueues->msgQueue.pop_message(out_msg);
    if (res) {
      Log::log("USERDATA: " + out_msg);

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
