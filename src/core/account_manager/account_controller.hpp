#ifndef ACCOUNT_CONTROLLER_HPP
#define ACCOUNT_CONTROLLER_HPP

#include "core/controllers/user_data_stream_controller.hpp"
#include "core/parsers/account_rest_api_parser.hpp"
#include "core/parsers/user_data_stream_parser.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/fixed_num.hpp"
#include "core/utils/helper_utils.hpp"
#include "core/utils/queue.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>

struct BalanceAsset {
  std::string assetName{};
  Fixed walletBalance{};
  Fixed crossWalletBalance{};
};

struct PositionMeta {
  Fixed realizedPnL{};
  Fixed unrealizedPnL{};
  Fixed breakEvenPrice{};
};

struct PositionAsset {
  std::string symbol{};
  Fixed positionAmt{};
  Fixed entryPrice{};
  POSITION_SIDE positionSide{};

  PositionMeta positionMeta{};

  static std::string getSignature(const std::string &symbol,
                                  POSITION_SIDE posSide) {
    return symbol + "_" + type_to_str(POSITION_SIDE_STR, posSide);
  }
};

class AccountPositions {
public:
  void tryRemove(const std::string &posSignature);
  void tryEmplace(const std::string &posSignature,
                  std::function<void(PositionAsset &)> fn);

  const std::set<std::string> &getPositionsList() const;

private:
  void addToSet(const std::string &position);
  void removeFromSet(const std::string &position);

private:
  std::set<std::string> m_positionsList;
  // key - signature
  std::unordered_map<std::string, PositionAsset> m_positions;
  std::mutex m_posititionsMutex;
  std::mutex m_posititionsListMutex;
};

class AccountBalance {
public:
  void tryEmplace(const std::string &assetName,
                  std::function<void(BalanceAsset &)> fn);

  const std::set<std::string> &getBalancesList() const;

private:
  void addToSet(const std::string &asset);
  void removeFromSet(const std::string &asset);

private:
  std::set<std::string> m_balancesList;
  // key - assetName
  std::unordered_map<std::string, BalanceAsset> m_balance;
  std::mutex m_balanceMutex;
  std::mutex m_balanceListMutex;
};

struct SymbolConfig {
  uint32_t leverage = 1;
  MARGIN_TYPE marginType = MARGIN_TYPE::CROSSED;
  bool isActive = true;
};

struct AccountPermissions {
  bool canTrade = false;
  bool canDeposit = false;
  bool canWithdraw = false;
  uint32_t feeTier = 0;
};

struct RiskManagementConfig {
  uint32_t maxLeverage = 20;
  Fixed maxPositionSize{};
  Fixed maxTotalPositionValue{};
  bool autoReduceOnMarginCall = false;
};

class AccountConfig {
public:
  // Leverage management
  void updateLeverage(const std::string &symbol, uint32_t leverage);
  uint32_t getLeverageConfig(const std::string &symbol) const;

  // Multi-asset mode
  void updateMultiAssetMode(bool mode);
  bool getMultiAssetMode() const;

  // Symbol configuration
  void updateSymbolConfig(const std::string &symbol, const SymbolConfig &config);
  void setMarginType(const std::string &symbol, MARGIN_TYPE marginType);
  std::optional<SymbolConfig> getSymbolConfig(const std::string &symbol) const;
  MARGIN_TYPE getMarginType(const std::string &symbol) const;

  // Account permissions
  void updatePermissions(const AccountPermissions &permissions);
  AccountPermissions getPermissions() const;
  bool canTrade() const;

  // Risk management
  void updateRiskConfig(const RiskManagementConfig &config);
  RiskManagementConfig getRiskConfig() const;

  // Commission rates
  void updateCommissionRate(const std::string &symbol, Fixed maker, Fixed taker);
  std::pair<Fixed, Fixed> getCommissionRate(const std::string &symbol) const;

  // Active symbols
  std::set<std::string> getActiveSymbols() const;
  void setSymbolActive(const std::string &symbol, bool active);

private:
  // Symbol configurations (leverage, margin type, etc.)
  std::unordered_map<std::string, SymbolConfig> m_symbolConfigs{};

  // Commission rates per symbol (maker, taker)
  std::unordered_map<std::string, std::pair<Fixed, Fixed>> m_commissionRates{};

  // Account-level settings
  bool m_multiAssetMode = false;
  AccountPermissions m_permissions{};
  RiskManagementConfig m_riskConfig{};

  mutable std::mutex m_configMutex;
};

class AccountController : public UserData::IUserEventListener {
public:
  AccountController();
  ~AccountController();

  uint64_t getLastUpdateTime() const { return m_lastUpdateTime; }

  const std::set<std::string> &getBalancesList() const;
  const std::set<std::string> &getPositionsList() const;

  // REST API response processing
  void processRestApiResponse(const AccountRestApi::ParsedAccountRestApi &response);

  // Access to sub-components
  const AccountConfig& getConfig() const { return *m_config; }
  AccountConfig& getConfig() { return *m_config; }

  // UserEventListener interface
  void enqueue(UserData::ParsedUserData event) override;
  void start() override;
  void stop() override;

private:
  void _runProcessingThread();
  void _listenToUpdates();

  void updateOrCreateAccountInfo(const UserData::ParsedUserData &event);

  void updateBalance(const UserData::AccountUpdateEvent &event);
  void updatePositions(const UserData::AccountUpdateEvent &event);
  void updateConfig(const UserData::AccountConfigUpdateEvent &event);

private:
  void _updateLastUpdateTime();

private:
  std::unique_ptr<AccountConfig> m_config;
  std::unique_ptr<AccountBalance> m_balance;
  std::unique_ptr<AccountPositions> m_positions;

  std::unique_ptr<Queue<UserData::ParsedUserData>> m_eventQueue;
  std::atomic_bool m_isQueueActive{false};

  std::unique_ptr<Thread> m_processingThread;

  uint64_t m_lastUpdateTime;
};

#endif // ACCOUNT_CONTROLLER_HPP
