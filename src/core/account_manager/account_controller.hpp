#ifndef ACCOUNT_CONTROLLER_HPP
#define ACCOUNT_CONTROLLER_HPP

#include "core/controllers/user_data_stream_controller.hpp"
#include "core/parsers/user_data_stream_parser.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/fixed_num.hpp"
#include "core/utils/helper_utils.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <string>

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

class AccountConfig {
public:
  void updateLeverage(const std::string &symbol, uint32_t leverage);
  void updateMultiAssetMode(bool mode);

  uint32_t getLeverageConfig(const std::string &symbol) const;
  bool getMultiAssetMode() const;

private:
  // key = symbol, value = leverage
  std::unordered_map<std::string, uint32_t> m_leverageConfig{};
  bool m_multiAssetModeUpdate = false;

  mutable std::mutex m_configMutex;
};

class AccountController : public UserData::IUserEventListener {
public:
  AccountController();

  uint64_t getLastUpdateTime() const { return m_lastUpdateTime; }

  const std::set<std::string> &getBalancesList() const;
  const std::set<std::string> &getPositionsList() const;

  void onEvent(const UserData::ParsedUserData &event) override;

private:
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

  uint64_t m_lastUpdateTime;
};

#endif // ACCOUNT_CONTROLLER_HPP
