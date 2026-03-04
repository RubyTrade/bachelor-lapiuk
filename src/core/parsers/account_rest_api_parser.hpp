#ifndef ACCOUNT_REST_API_PARSER_HPP
#define ACCOUNT_REST_API_PARSER_HPP

#include "core/controllers/account_rest_api_utils.hpp"
#include "core/parsers/common_parser_utils.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/fixed_num.hpp"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace AccountRestApi {

// ============================================================================
// Account Info Response
// ============================================================================
struct AccountInfoResponse {
  ACCOUNT_API_TYPE apiType = ACCOUNT_API_TYPE::ACCOUNT_INFO;

  struct Asset {
    std::string asset{};
    Fixed walletBalance{};
    Fixed unrealizedProfit{};
    Fixed marginBalance{};
    Fixed maintMargin{};
    Fixed initialMargin{};
    Fixed positionInitialMargin{};
    Fixed openOrderInitialMargin{};
    Fixed crossWalletBalance{};
    Fixed crossUnPnl{};
    Fixed availableBalance{};
    Fixed maxWithdrawAmount{};
    bool marginAvailable = false;
    uint64_t updateTime = 0;
  };

  struct Position {
    std::string symbol{};
    Fixed initialMargin{};
    Fixed maintMargin{};
    Fixed unrealizedProfit{};
    Fixed positionInitialMargin{};
    Fixed openOrderInitialMargin{};
    uint32_t leverage = 0;
    bool isolated = false;
    Fixed entryPrice{};
    Fixed maxNotional{};
    Fixed bidNotional{};
    Fixed askNotional{};
    POSITION_SIDE positionSide{};
    Fixed positionAmt{};
    Fixed notional{};
    Fixed isolatedWallet{};
    uint64_t updateTime = 0;
    Fixed breakEvenPrice{};
  };

  uint32_t feeTier = 0;
  bool canTrade = false;
  bool canDeposit = false;
  bool canWithdraw = false;
  uint64_t updateTime = 0;
  bool multiAssetsMargin = false;
  Fixed totalInitialMargin{};
  Fixed totalMaintMargin{};
  Fixed totalWalletBalance{};
  Fixed totalUnrealizedProfit{};
  Fixed totalMarginBalance{};
  Fixed totalPositionInitialMargin{};
  Fixed totalOpenOrderInitialMargin{};
  Fixed totalCrossWalletBalance{};
  Fixed totalCrossUnPnl{};
  Fixed availableBalance{};
  Fixed maxWithdrawAmount{};

  std::vector<Asset> assets;
  std::vector<Position> positions;
};

// ============================================================================
// Account Balance Response
// ============================================================================
struct AccountBalanceResponse {
  ACCOUNT_API_TYPE apiType = ACCOUNT_API_TYPE::ACCOUNT_BALANCE;

  struct Balance {
    std::string accountAlias{};
    std::string asset{};
    Fixed balance{};
    Fixed crossWalletBalance{};
    Fixed crossUnPnl{};
    Fixed availableBalance{};
    Fixed maxWithdrawAmount{};
    bool marginAvailable = false;
    uint64_t updateTime = 0;
  };

  std::vector<Balance> balances;
};

// ============================================================================
// Commission Rate Response
// ============================================================================
struct CommissionRateResponse {
  ACCOUNT_API_TYPE apiType = ACCOUNT_API_TYPE::COMMISSION_RATE;

  std::string symbol{};
  Fixed makerCommissionRate{};
  Fixed takerCommissionRate{};
};

using ParsedAccountRestApi =
    std::variant<AccountInfoResponse, AccountBalanceResponse,
                 CommissionRateResponse, ErrorParse>;

class AccountRestApiParser {
public:
  static ParsedAccountRestApi parse(const RestApiMessage &msg);
};

class AccountInfoParser {
public:
  static ParsedAccountRestApi parse(const RestApiMessage &msg);

private:
  // Common error keys
  static constexpr std::string_view CODE = "code";
  static constexpr std::string_view MSG = "msg";

  // Account Info keys
  static constexpr std::string_view FEE_TIER = "feeTier";
  static constexpr std::string_view CAN_TRADE = "canTrade";
  static constexpr std::string_view CAN_DEPOSIT = "canDeposit";
  static constexpr std::string_view CAN_WITHDRAW = "canWithdraw";
  static constexpr std::string_view UPDATE_TIME = "updateTime";
  static constexpr std::string_view MULTI_ASSETS_MARGIN = "multiAssetsMargin";
  static constexpr std::string_view TOTAL_INITIAL_MARGIN = "totalInitialMargin";
  static constexpr std::string_view TOTAL_MAINT_MARGIN = "totalMaintMargin";
  static constexpr std::string_view TOTAL_WALLET_BALANCE = "totalWalletBalance";
  static constexpr std::string_view TOTAL_UNREALIZED_PROFIT =
      "totalUnrealizedProfit";
  static constexpr std::string_view TOTAL_MARGIN_BALANCE = "totalMarginBalance";
  static constexpr std::string_view TOTAL_POSITION_INITIAL_MARGIN =
      "totalPositionInitialMargin";
  static constexpr std::string_view TOTAL_OPEN_ORDER_INITIAL_MARGIN =
      "totalOpenOrderInitialMargin";
  static constexpr std::string_view TOTAL_CROSS_WALLET_BALANCE =
      "totalCrossWalletBalance";
  static constexpr std::string_view TOTAL_CROSS_UN_PNL = "totalCrossUnPnl";
  static constexpr std::string_view AVAILABLE_BALANCE = "availableBalance";
  static constexpr std::string_view MAX_WITHDRAW_AMOUNT = "maxWithdrawAmount";
  static constexpr std::string_view ASSETS = "assets";
  static constexpr std::string_view POSITIONS = "positions";

  // Asset keys
  static constexpr std::string_view ASSET = "asset";
  static constexpr std::string_view WALLET_BALANCE = "walletBalance";
  static constexpr std::string_view UNREALIZED_PROFIT = "unrealizedProfit";
  static constexpr std::string_view MARGIN_BALANCE = "marginBalance";
  static constexpr std::string_view MAINT_MARGIN = "maintMargin";
  static constexpr std::string_view INITIAL_MARGIN = "initialMargin";
  static constexpr std::string_view POSITION_INITIAL_MARGIN =
      "positionInitialMargin";
  static constexpr std::string_view OPEN_ORDER_INITIAL_MARGIN =
      "openOrderInitialMargin";
  static constexpr std::string_view CROSS_WALLET_BALANCE = "crossWalletBalance";
  static constexpr std::string_view CROSS_UN_PNL = "crossUnPnl";
  static constexpr std::string_view MARGIN_AVAILABLE = "marginAvailable";

  // Position keys
  static constexpr std::string_view SYMBOL = "symbol";
  static constexpr std::string_view LEVERAGE = "leverage";
  static constexpr std::string_view ISOLATED = "isolated";
  static constexpr std::string_view ENTRY_PRICE = "entryPrice";
  static constexpr std::string_view MAX_NOTIONAL = "maxNotional";
  static constexpr std::string_view BID_NOTIONAL = "bidNotional";
  static constexpr std::string_view ASK_NOTIONAL = "askNotional";
  static constexpr std::string_view POSITION_SIDE = "positionSide";
  static constexpr std::string_view POSITION_AMT = "positionAmt";
  static constexpr std::string_view NOTIONAL = "notional";
  static constexpr std::string_view ISOLATED_WALLET = "isolatedWallet";
  static constexpr std::string_view BREAK_EVEN_PRICE = "breakEvenPrice";
};

class AccountBalanceParser {
public:
  static ParsedAccountRestApi parse(const RestApiMessage &msg);

private:
  // Common error keys
  static constexpr std::string_view CODE = "code";
  static constexpr std::string_view MSG = "msg";

  // Balance keys
  static constexpr std::string_view ACCOUNT_ALIAS = "accountAlias";
  static constexpr std::string_view ASSET = "asset";
  static constexpr std::string_view BALANCE = "balance";
  static constexpr std::string_view CROSS_WALLET_BALANCE = "crossWalletBalance";
  static constexpr std::string_view CROSS_UN_PNL = "crossUnPnl";
  static constexpr std::string_view AVAILABLE_BALANCE = "availableBalance";
  static constexpr std::string_view MAX_WITHDRAW_AMOUNT = "maxWithdrawAmount";
  static constexpr std::string_view MARGIN_AVAILABLE = "marginAvailable";
  static constexpr std::string_view UPDATE_TIME = "updateTime";
};

class CommissionRateParser {
public:
  static ParsedAccountRestApi parse(const RestApiMessage &msg);

private:
  // Common error keys
  static constexpr std::string_view CODE = "code";
  static constexpr std::string_view MSG = "msg";

  // Commission Rate keys
  static constexpr std::string_view SYMBOL = "symbol";
  static constexpr std::string_view MAKER_COMMISSION_RATE = "makerCommissionRate";
  static constexpr std::string_view TAKER_COMMISSION_RATE = "takerCommissionRate";
};

} // namespace AccountRestApi

#endif // ACCOUNT_REST_API_PARSER_HPP
