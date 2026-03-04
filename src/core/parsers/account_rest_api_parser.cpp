#include "account_rest_api_parser.hpp"
#include "core/controllers/account_rest_api_utils.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/json.hpp"

using namespace AccountRestApi;

/* static */ ParsedAccountRestApi
AccountRestApiParser::parse(const RestApiMessage &msg) {
  if (msg.data.is_empty())
    return ErrorParse{"RestApiMessage data is empty"};

  switch (msg.apiType) {
  case ACCOUNT_API_TYPE::ACCOUNT_INFO:
    return AccountInfoParser::parse(msg);
  case ACCOUNT_API_TYPE::ACCOUNT_BALANCE:
    return AccountBalanceParser::parse(msg);
  case ACCOUNT_API_TYPE::COMMISSION_RATE:
    return CommissionRateParser::parse(msg);
  default:
    return ErrorParse{"RestApiMessage type is invalid"};
  }
}

/* static */ ParsedAccountRestApi
AccountInfoParser::parse(const RestApiMessage &msg) {
  try {
    JSONQuery json = msg.data;
    AccountInfoResponse data;

    if (json.is_empty()) {
      return ErrorParse{"JSON is empty"};
    }

    // Check for error response
    if (auto code = json.get_value(std::string(CODE));
        code && code->is_number_integer()) {
      if (auto msgVal = json.get_value(std::string(MSG));
          msgVal && msgVal->is_string()) {
        return ErrorParse{"Error " + std::to_string(code->get<int>()) + ": " +
                          msgVal->get<std::string>()};
      }
    }

    // Parse basic fields
    if (auto val = json.get_value(std::string(FEE_TIER));
        val && val->is_number_unsigned()) {
      data.feeTier = val->get<uint32_t>();
    }

    if (auto val = json.get_value(std::string(CAN_TRADE));
        val && val->is_boolean()) {
      data.canTrade = val->get<bool>();
    }

    if (auto val = json.get_value(std::string(CAN_DEPOSIT));
        val && val->is_boolean()) {
      data.canDeposit = val->get<bool>();
    }

    if (auto val = json.get_value(std::string(CAN_WITHDRAW));
        val && val->is_boolean()) {
      data.canWithdraw = val->get<bool>();
    }

    if (auto val = json.get_value(std::string(UPDATE_TIME));
        val && val->is_number_unsigned()) {
      data.updateTime = val->get<uint64_t>();
    }

    if (auto val = json.get_value(std::string(MULTI_ASSETS_MARGIN));
        val && val->is_boolean()) {
      data.multiAssetsMargin = val->get<bool>();
    }

    // Parse total margin fields
    if (auto val = json.get_value(std::string(TOTAL_INITIAL_MARGIN));
        val && val->is_string()) {
      data.totalInitialMargin =
          Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(TOTAL_MAINT_MARGIN));
        val && val->is_string()) {
      data.totalMaintMargin =
          Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(TOTAL_WALLET_BALANCE));
        val && val->is_string()) {
      data.totalWalletBalance =
          Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(TOTAL_UNREALIZED_PROFIT));
        val && val->is_string()) {
      data.totalUnrealizedProfit =
          Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(TOTAL_MARGIN_BALANCE));
        val && val->is_string()) {
      data.totalMarginBalance =
          Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(TOTAL_POSITION_INITIAL_MARGIN));
        val && val->is_string()) {
      data.totalPositionInitialMargin =
          Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(TOTAL_OPEN_ORDER_INITIAL_MARGIN));
        val && val->is_string()) {
      data.totalOpenOrderInitialMargin =
          Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(TOTAL_CROSS_WALLET_BALANCE));
        val && val->is_string()) {
      data.totalCrossWalletBalance =
          Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(TOTAL_CROSS_UN_PNL));
        val && val->is_string()) {
      data.totalCrossUnPnl =
          Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(AVAILABLE_BALANCE));
        val && val->is_string()) {
      data.availableBalance =
          Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(MAX_WITHDRAW_AMOUNT));
        val && val->is_string()) {
      data.maxWithdrawAmount =
          Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    // Parse assets array
    if (auto assetsVal = json.get_value(std::string(ASSETS));
        assetsVal && assetsVal->is_array()) {
      for (const auto &assetObj : *assetsVal) {
        JSONQuery assetJson(assetObj);
        AccountInfoResponse::Asset asset;

        if (auto val = assetJson.get_value(std::string(ASSET));
            val && val->is_string()) {
          asset.asset = val->get<std::string>();
        }

        if (auto val = assetJson.get_value(std::string(WALLET_BALANCE));
            val && val->is_string()) {
          asset.walletBalance =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val = assetJson.get_value(std::string(UNREALIZED_PROFIT));
            val && val->is_string()) {
          asset.unrealizedProfit =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val = assetJson.get_value(std::string(MARGIN_BALANCE));
            val && val->is_string()) {
          asset.marginBalance =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val = assetJson.get_value(std::string(MAINT_MARGIN));
            val && val->is_string()) {
          asset.maintMargin =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val = assetJson.get_value(std::string(INITIAL_MARGIN));
            val && val->is_string()) {
          asset.initialMargin =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val =
                assetJson.get_value(std::string(POSITION_INITIAL_MARGIN));
            val && val->is_string()) {
          asset.positionInitialMargin =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val =
                assetJson.get_value(std::string(OPEN_ORDER_INITIAL_MARGIN));
            val && val->is_string()) {
          asset.openOrderInitialMargin =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val = assetJson.get_value(std::string(CROSS_WALLET_BALANCE));
            val && val->is_string()) {
          asset.crossWalletBalance =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val = assetJson.get_value(std::string(CROSS_UN_PNL));
            val && val->is_string()) {
          asset.crossUnPnl =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val = assetJson.get_value(std::string(AVAILABLE_BALANCE));
            val && val->is_string()) {
          asset.availableBalance =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val = assetJson.get_value(std::string(MAX_WITHDRAW_AMOUNT));
            val && val->is_string()) {
          asset.maxWithdrawAmount =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val = assetJson.get_value(std::string(MARGIN_AVAILABLE));
            val && val->is_boolean()) {
          asset.marginAvailable = val->get<bool>();
        }

        if (auto val = assetJson.get_value(std::string(UPDATE_TIME));
            val && val->is_number_unsigned()) {
          asset.updateTime = val->get<uint64_t>();
        }

        data.assets.push_back(std::move(asset));
      }
    }

    // Parse positions array
    if (auto positionsVal = json.get_value(std::string(POSITIONS));
        positionsVal && positionsVal->is_array()) {
      for (const auto &posObj : *positionsVal) {
        JSONQuery posJson(posObj);
        AccountInfoResponse::Position position;

        if (auto val = posJson.get_value(std::string(SYMBOL));
            val && val->is_string()) {
          position.symbol = val->get<std::string>();
        }

        if (auto val = posJson.get_value(std::string(INITIAL_MARGIN));
            val && val->is_string()) {
          position.initialMargin =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val = posJson.get_value(std::string(MAINT_MARGIN));
            val && val->is_string()) {
          position.maintMargin =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val = posJson.get_value(std::string(UNREALIZED_PROFIT));
            val && val->is_string()) {
          position.unrealizedProfit =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val = posJson.get_value(std::string(POSITION_INITIAL_MARGIN));
            val && val->is_string()) {
          position.positionInitialMargin =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val =
                posJson.get_value(std::string(OPEN_ORDER_INITIAL_MARGIN));
            val && val->is_string()) {
          position.openOrderInitialMargin =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val = posJson.get_value(std::string(LEVERAGE));
            val && val->is_number_unsigned()) {
          position.leverage = val->get<uint32_t>();
        }

        if (auto val = posJson.get_value(std::string(ISOLATED));
            val && val->is_boolean()) {
          position.isolated = val->get<bool>();
        }

        if (auto val = posJson.get_value(std::string(ENTRY_PRICE));
            val && val->is_string()) {
          position.entryPrice =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val = posJson.get_value(std::string(MAX_NOTIONAL));
            val && val->is_string()) {
          position.maxNotional =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val = posJson.get_value(std::string(BID_NOTIONAL));
            val && val->is_string()) {
          position.bidNotional =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val = posJson.get_value(std::string(ASK_NOTIONAL));
            val && val->is_string()) {
          position.askNotional =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val = posJson.get_value(std::string(POSITION_SIDE));
            val && val->is_string()) {
          position.positionSide = str_to_type(
              POSITION_SIDE_STR, val->get_ref<const std::string &>());
        }

        if (auto val = posJson.get_value(std::string(POSITION_AMT));
            val && val->is_string()) {
          position.positionAmt =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val = posJson.get_value(std::string(NOTIONAL));
            val && val->is_string()) {
          position.notional =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val = posJson.get_value(std::string(ISOLATED_WALLET));
            val && val->is_string()) {
          position.isolatedWallet =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        if (auto val = posJson.get_value(std::string(UPDATE_TIME));
            val && val->is_number_unsigned()) {
          position.updateTime = val->get<uint64_t>();
        }

        if (auto val = posJson.get_value(std::string(BREAK_EVEN_PRICE));
            val && val->is_string()) {
          position.breakEvenPrice =
              Fixed::str_to_fixed(val->get_ref<const std::string &>());
        }

        data.positions.push_back(std::move(position));
      }
    }

    return data;
  } catch (const nlohmann::json::exception &e) {
    return ErrorParse{"JSON exception in AccountInfoParser: " +
                      std::string(e.what())};
  }
}

/* static */ ParsedAccountRestApi
AccountBalanceParser::parse(const RestApiMessage &msg) {
  try {
    JSONQuery json = msg.data;
    AccountBalanceResponse data;

    if (json.is_empty()) {
      return ErrorParse{"JSON is empty"};
    }

    // Check for error response
    if (auto code = json.get_value(std::string(CODE));
        code && code->is_number_integer()) {
      if (auto msgVal = json.get_value(std::string(MSG));
          msgVal && msgVal->is_string()) {
        return ErrorParse{"Error " + std::to_string(code->get<int>()) + ": " +
                          msgVal->get<std::string>()};
      }
    }

    // The response is an array of balance objects
    if (!json.is_array()) {
      return ErrorParse{"Expected array of balances"};
    }

    auto jsonArray = json.get_array();
    if (!jsonArray.has_value()) {
      return ErrorParse{"Cannot get balance array"};
    }

    for (const auto &balanceObj : jsonArray.value()) {
      JSONQuery balanceJson(balanceObj);
      AccountBalanceResponse::Balance balance;

      if (auto val = balanceJson.get_value(std::string(ACCOUNT_ALIAS));
          val && val->is_string()) {
        balance.accountAlias = val->get<std::string>();
      }

      if (auto val = balanceJson.get_value(std::string(ASSET));
          val && val->is_string()) {
        balance.asset = val->get<std::string>();
      }

      if (auto val = balanceJson.get_value(std::string(BALANCE));
          val && val->is_string()) {
        balance.balance =
            Fixed::str_to_fixed(val->get_ref<const std::string &>());
      }

      if (auto val = balanceJson.get_value(std::string(CROSS_WALLET_BALANCE));
          val && val->is_string()) {
        balance.crossWalletBalance =
            Fixed::str_to_fixed(val->get_ref<const std::string &>());
      }

      if (auto val = balanceJson.get_value(std::string(CROSS_UN_PNL));
          val && val->is_string()) {
        balance.crossUnPnl =
            Fixed::str_to_fixed(val->get_ref<const std::string &>());
      }

      if (auto val = balanceJson.get_value(std::string(AVAILABLE_BALANCE));
          val && val->is_string()) {
        balance.availableBalance =
            Fixed::str_to_fixed(val->get_ref<const std::string &>());
      }

      if (auto val = balanceJson.get_value(std::string(MAX_WITHDRAW_AMOUNT));
          val && val->is_string()) {
        balance.maxWithdrawAmount =
            Fixed::str_to_fixed(val->get_ref<const std::string &>());
      }

      if (auto val = balanceJson.get_value(std::string(MARGIN_AVAILABLE));
          val && val->is_boolean()) {
        balance.marginAvailable = val->get<bool>();
      }

      if (auto val = balanceJson.get_value(std::string(UPDATE_TIME));
          val && val->is_number_unsigned()) {
        balance.updateTime = val->get<uint64_t>();
      }

      data.balances.push_back(std::move(balance));
    }

    return data;
  } catch (const nlohmann::json::exception &e) {
    return ErrorParse{"JSON exception in AccountBalanceParser: " +
                      std::string(e.what())};
  }
}

/* static */ ParsedAccountRestApi
CommissionRateParser::parse(const RestApiMessage &msg) {
  try {
    JSONQuery json = msg.data;
    CommissionRateResponse data;

    if (json.is_empty()) {
      return ErrorParse{"JSON is empty"};
    }

    // Check for error response
    if (auto code = json.get_value(std::string(CODE));
        code && code->is_number_integer()) {
      if (auto msgVal = json.get_value(std::string(MSG));
          msgVal && msgVal->is_string()) {
        return ErrorParse{"Error " + std::to_string(code->get<int>()) + ": " +
                          msgVal->get<std::string>()};
      }
    }

    if (auto val = json.get_value(std::string(SYMBOL));
        val && val->is_string()) {
      data.symbol = val->get<std::string>();
    }

    if (auto val = json.get_value(std::string(MAKER_COMMISSION_RATE));
        val && val->is_string()) {
      data.makerCommissionRate =
          Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(TAKER_COMMISSION_RATE));
        val && val->is_string()) {
      data.takerCommissionRate =
          Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    return data;
  } catch (const nlohmann::json::exception &e) {
    return ErrorParse{"JSON exception in CommissionRateParser: " +
                      std::string(e.what())};
  }
}
