#include "core/account_manager/account_controller.hpp"
#include "core/parsers/user_data_stream_parser.hpp"
#include "core/utils/log.hpp"
#include <memory>
#include <mutex>
#include <variant>

void AccountPositions::tryRemove(const std::string &posSignature) {
  std::lock_guard<std::mutex> lock(m_posititionsMutex);

  m_positions.erase(posSignature);
  removeFromSet(posSignature);
}

void AccountPositions::tryEmplace(const std::string &posSignature,
                                  std::function<void(PositionAsset &)> fn) {
  std::lock_guard<std::mutex> lock(m_posititionsMutex);

  auto [it, inserted] = m_positions.try_emplace(posSignature);
  fn(it->second);
  addToSet(posSignature);
}

void AccountBalance::tryEmplace(const std::string &assetName,
                                std::function<void(BalanceAsset &)> fn) {
  std::lock_guard<std::mutex> lock(m_balanceMutex);

  auto [it, inserted] = m_balance.try_emplace(assetName);
  fn(it->second);
  addToSet(assetName);
}

void AccountPositions::addToSet(const std::string &position) {
  std::lock_guard<std::mutex> lock(m_posititionsListMutex);

  m_positionsList.insert(position);
}

void AccountPositions::removeFromSet(const std::string &position) {
  std::lock_guard<std::mutex> lock(m_posititionsListMutex);

  m_positionsList.erase(position);
}

void AccountBalance::addToSet(const std::string &asset) {
  std::lock_guard<std::mutex> lock(m_balanceListMutex);

  m_balancesList.insert(asset);
}

void AccountBalance::removeFromSet(const std::string &asset) {
  std::lock_guard<std::mutex> lock(m_balanceListMutex);

  m_balancesList.erase(asset);
}

const std::set<std::string> &AccountBalance::getBalancesList() const {
  return m_balancesList;
}

const std::set<std::string> &AccountPositions::getPositionsList() const {
  return m_positionsList;
}

const std::set<std::string> &AccountController::getBalancesList() const {
  return m_balance->getBalancesList();
}

const std::set<std::string> &AccountController::getPositionsList() const {
  return m_positions->getPositionsList();
}

void AccountConfig::updateLeverage(const std::string &symbol,
                                   uint32_t leverage) {
  std::lock_guard<std::mutex> lock(m_configMutex);
  m_leverageConfig[symbol] = leverage;
}

void AccountConfig::updateMultiAssetMode(bool mode) {
  std::lock_guard<std::mutex> lock(m_configMutex);
  m_multiAssetModeUpdate = mode;
}

uint32_t AccountConfig::getLeverageConfig(const std::string &symbol) const {
  std::lock_guard<std::mutex> lock(m_configMutex);
  auto it = m_leverageConfig.find(symbol);
  if (it != m_leverageConfig.end()) {
    return it->second;
  }
  return 0;
}

bool AccountConfig::getMultiAssetMode() const {
  std::lock_guard<std::mutex> lock(m_configMutex);
  return m_multiAssetModeUpdate;
}

// AccountController
AccountController::AccountController()
    : m_balance(std::make_unique<AccountBalance>()),
      m_positions(std::make_unique<AccountPositions>()),
      m_config(std::make_unique<AccountConfig>()),
      m_eventQueue(std::make_unique<Queue<UserData::ParsedUserData>>()),
      m_processingThread(std::make_unique<Thread>()) {
  _updateLastUpdateTime();
  _runProcessingThread();
}

void AccountController::enqueue(UserData::ParsedUserData event) {
  if (!std::holds_alternative<ErrorParse>(event))
    m_eventQueue->push_message(std::move(event));

  return;
}

void AccountController::_runProcessingThread() {
  m_processingThread->start(&AccountController::_listenToUpdates, this);
}

void AccountController::_listenToUpdates() {
  while (true) {
    UserData::ParsedUserData out_msg;
    bool res = m_eventQueue->pop_message(out_msg);
    if (res) {
      updateOrCreateAccountInfo(std::move(out_msg));
    }
  }
}

void AccountController::updateOrCreateAccountInfo(
    const UserData::ParsedUserData &event) {
  if (std::holds_alternative<UserData::AccountUpdateEvent>(event)) {
    updateBalance(std::get<UserData::AccountUpdateEvent>(event));
    updatePositions(std::get<UserData::AccountUpdateEvent>(event));
  } else if (std::holds_alternative<UserData::AccountConfigUpdateEvent>(
                 event)) {
    updateConfig(std::get<UserData::AccountConfigUpdateEvent>(event));
  }

  // If we hit else,
  // that means, no update was done
  else {
    return;
  }

  _updateLastUpdateTime();
}

void AccountController::updateBalance(
    const UserData::AccountUpdateEvent &event) {
  for (auto &position : event.positions) {
    std::string signature =
        PositionAsset::getSignature(position.symbol, position.positionSide);

    if (position.positionAmt == Fixed(0, position.positionAmt.scale())) {
      m_positions->tryRemove(signature);
    } else {
      m_positions->tryEmplace(signature, [&position](PositionAsset &asset) {
        asset.symbol = position.symbol;
        asset.positionSide = position.positionSide;
        asset.positionAmt = position.positionAmt;
        asset.entryPrice = position.entryPrice;

        asset.positionMeta.breakEvenPrice = position.breakEvenPrice;
        asset.positionMeta.realizedPnL = position.realizedPnL;
        asset.positionMeta.unrealizedPnL = position.unrealizedPnL;
      });
    }
  }
}

void AccountController::updatePositions(
    const UserData::AccountUpdateEvent &event) {
  for (auto &balanceAsset : event.balances) {
    m_balance->tryEmplace(
        balanceAsset.asset, [&balanceAsset](BalanceAsset &asset) {
          asset.assetName = balanceAsset.asset;
          asset.crossWalletBalance = balanceAsset.crossWalletBalance;
          asset.walletBalance = balanceAsset.walletBalance;
        });
  }
}

void AccountController::updateConfig(
    const UserData::AccountConfigUpdateEvent &event) {
  if (event.leverageUpdate) {
    m_config->updateLeverage(event.leverageUpdate->symbol,
                             event.leverageUpdate->leverage);
  }

  if (event.multiAssetModeUpdate) {
    m_config->updateMultiAssetMode(event.multiAssetModeUpdate->multiAssetsMode);
  }
}

void AccountController::_updateLastUpdateTime() {
  m_lastUpdateTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
}
