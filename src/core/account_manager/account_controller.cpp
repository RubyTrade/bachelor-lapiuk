#include "core/account_manager/account_controller.hpp"
#include "core/parsers/account_rest_api_parser.hpp"
#include "core/parsers/user_data_stream_parser.hpp"
#include "core/utils/constants.hpp"
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

std::optional<Fixed>
AccountBalance::getWalletBalance(const std::string &assetName) const {
  std::lock_guard<std::mutex> lock(m_balanceMutex);
  auto it = m_balance.find(assetName);
  if (it == m_balance.end())
    return std::nullopt;
  return it->second.walletBalance;
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

std::optional<Fixed>
AccountController::getAssetWalletBalance(const std::string &asset) const {
  return m_balance->getWalletBalance(asset);
}

void AccountConfig::updateLeverage(const std::string &symbol,
                                   uint32_t leverage) {
  std::lock_guard<std::mutex> lock(m_configMutex);
  m_symbolConfigs[symbol].leverage = leverage;
}

void AccountConfig::updateMultiAssetMode(bool mode) {
  std::lock_guard<std::mutex> lock(m_configMutex);
  m_multiAssetMode = mode;
}

uint32_t AccountConfig::getLeverageConfig(const std::string &symbol) const {
  std::lock_guard<std::mutex> lock(m_configMutex);
  auto it = m_symbolConfigs.find(symbol);
  if (it != m_symbolConfigs.end()) {
    return it->second.leverage;
  }
  return 1; // default leverage
}

bool AccountConfig::getMultiAssetMode() const {
  std::lock_guard<std::mutex> lock(m_configMutex);
  return m_multiAssetMode;
}

void AccountConfig::updateSymbolConfig(const std::string &symbol,
                                       const SymbolConfig &config) {
  std::lock_guard<std::mutex> lock(m_configMutex);
  m_symbolConfigs[symbol] = config;
}

void AccountConfig::setMarginType(const std::string &symbol,
                                  MARGIN_TYPE marginType) {
  std::lock_guard<std::mutex> lock(m_configMutex);
  m_symbolConfigs[symbol].marginType = marginType;
}

std::optional<SymbolConfig>
AccountConfig::getSymbolConfig(const std::string &symbol) const {
  std::lock_guard<std::mutex> lock(m_configMutex);
  auto it = m_symbolConfigs.find(symbol);
  if (it != m_symbolConfigs.end()) {
    return it->second;
  }
  return std::nullopt;
}

MARGIN_TYPE AccountConfig::getMarginType(const std::string &symbol) const {
  std::lock_guard<std::mutex> lock(m_configMutex);
  auto it = m_symbolConfigs.find(symbol);
  if (it != m_symbolConfigs.end()) {
    return it->second.marginType;
  }
  return MARGIN_TYPE::CROSSED; // default
}

void AccountConfig::updatePermissions(const AccountPermissions &permissions) {
  std::lock_guard<std::mutex> lock(m_configMutex);
  m_permissions = permissions;
}

AccountPermissions AccountConfig::getPermissions() const {
  std::lock_guard<std::mutex> lock(m_configMutex);
  return m_permissions;
}

bool AccountConfig::canTrade() const {
  std::lock_guard<std::mutex> lock(m_configMutex);
  return m_permissions.canTrade;
}

void AccountConfig::updateRiskConfig(const RiskManagementConfig &config) {
  std::lock_guard<std::mutex> lock(m_configMutex);
  m_riskConfig = config;
}

RiskManagementConfig AccountConfig::getRiskConfig() const {
  std::lock_guard<std::mutex> lock(m_configMutex);
  return m_riskConfig;
}

void AccountConfig::updateCommissionRate(const std::string &symbol, Fixed maker,
                                         Fixed taker) {
  std::lock_guard<std::mutex> lock(m_configMutex);
  m_commissionRates[symbol] = {maker, taker};
}

std::pair<Fixed, Fixed>
AccountConfig::getCommissionRate(const std::string &symbol) const {
  std::lock_guard<std::mutex> lock(m_configMutex);
  auto it = m_commissionRates.find(symbol);
  if (it != m_commissionRates.end()) {
    return it->second;
  }
  // Default commission rates
  return {Fixed(0, 4), Fixed(0, 4)};
}

std::set<std::string> AccountConfig::getActiveSymbols() const {
  std::lock_guard<std::mutex> lock(m_configMutex);
  std::set<std::string> activeSymbols;
  for (const auto &[symbol, config] : m_symbolConfigs) {
    if (config.isActive) {
      activeSymbols.insert(symbol);
    }
  }
  return activeSymbols;
}

void AccountConfig::setSymbolActive(const std::string &symbol, bool active) {
  std::lock_guard<std::mutex> lock(m_configMutex);
  m_symbolConfigs[symbol].isActive = active;
}

// AccountController
AccountController::AccountController()
    : m_balance(std::make_unique<AccountBalance>()),
      m_positions(std::make_unique<AccountPositions>()),
      m_config(std::make_unique<AccountConfig>()),
      m_eventQueue(std::make_unique<Queue<UserData::ParsedUserData>>()),
      m_processingThread(std::make_unique<Thread>()) {
  _updateLastUpdateTime();
}

AccountController::~AccountController() { stop(); }

void AccountController::enqueue(UserData::ParsedUserData event) {
  if (!std::holds_alternative<ErrorParse>(event))
    m_eventQueue->push_message(std::move(event));

  return;
}

void AccountController::start() {
  m_isQueueActive = true;
  m_processingThread->start(&AccountController::_listenToUpdates, this);
}

void AccountController::stop() {
  m_isQueueActive = false;
  m_eventQueue->stop_queue();
  m_processingThread->stop();
}

void AccountController::_listenToUpdates() {
  while (m_isQueueActive) {
    UserData::ParsedUserData out_msg;
    if (!m_eventQueue->pop_message(out_msg)) {
      break;
    }

    updateOrCreateAccountInfo(std::move(out_msg));
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

void AccountController::processRestApiResponse(
    const AccountRestApi::ParsedAccountRestApi &response) {

  if (std::holds_alternative<ErrorParse>(response)) {
    auto error = std::get<ErrorParse>(response);
    Log::log_err("REST API response parsing error: " + error.parse_error);
    return;
  }

  // Process AccountInfo response
  if (std::holds_alternative<AccountRestApi::AccountInfoResponse>(response)) {
    const auto &info = std::get<AccountRestApi::AccountInfoResponse>(response);

    // Update permissions
    AccountPermissions permissions;
    permissions.canTrade = info.canTrade;
    permissions.canDeposit = info.canDeposit;
    permissions.canWithdraw = info.canWithdraw;
    permissions.feeTier = info.feeTier;
    m_config->updatePermissions(permissions);

    // Update multi-asset mode
    m_config->updateMultiAssetMode(info.multiAssetsMargin);

    // Update balances from assets
    for (const auto &asset : info.assets) {
      m_balance->tryEmplace(asset.asset, [&asset](BalanceAsset &balance) {
        balance.assetName = asset.asset;
        balance.walletBalance = asset.walletBalance;
        balance.crossWalletBalance = asset.crossWalletBalance;
      });
    }

    // Update positions
    for (const auto &position : info.positions) {
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
          asset.positionMeta.unrealizedPnL = position.unrealizedProfit;
          // realizedPnL not available in position info
        });

        // Update symbol config with leverage
        if (position.leverage > 0) {
          m_config->updateLeverage(position.symbol, position.leverage);
        }

        // Determine margin type from position
        MARGIN_TYPE marginType =
            position.isolated ? MARGIN_TYPE::ISOLATED : MARGIN_TYPE::CROSSED;
        m_config->setMarginType(position.symbol, marginType);
      }
    }

    _updateLastUpdateTime();
  }

  // Process AccountBalance response
  else if (std::holds_alternative<AccountRestApi::AccountBalanceResponse>(
               response)) {
    const auto &balanceResp =
        std::get<AccountRestApi::AccountBalanceResponse>(response);

    for (const auto &balance : balanceResp.balances) {
      m_balance->tryEmplace(balance.asset, [&balance](BalanceAsset &asset) {
        asset.assetName = balance.asset;
        asset.walletBalance = balance.balance;
        asset.crossWalletBalance = balance.crossWalletBalance;
      });
    }

    _updateLastUpdateTime();
  }

  // Process CommissionRate response
  else if (std::holds_alternative<AccountRestApi::CommissionRateResponse>(
               response)) {
    const auto &commissionResp =
        std::get<AccountRestApi::CommissionRateResponse>(response);

    m_config->updateCommissionRate(commissionResp.symbol,
                                   commissionResp.makerCommissionRate,
                                   commissionResp.takerCommissionRate);

    _updateLastUpdateTime();
  }
}
