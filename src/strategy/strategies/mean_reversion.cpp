#include "mean_reversion.hpp"
#include "core/controllers/trading_stream_utils.hpp"
#include "core/parsers/market_data_parser.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/fixed_num.hpp"
#include <optional>

// TODO: consider adding z-score and std dev
SIGNAL MeanReversion::produce_signal() {
  Fixed vwap = _calculate_vwap();
  if (vwap == Fixed{0})
    return SIGNAL::HOLD;

  Fixed delta_rel = (vwap == 0) ? 0 : (m_last_agg_trade.price - vwap) / vwap;

  if (order_pool.is_flat()) {
    if (delta_rel > VWAP_DELTA)
      return SIGNAL::SHORT_ENTRY;

    if (delta_rel < VWAP_DELTA * Fixed{-1})
      return SIGNAL::LONG_ENTRY;
  } else {
    if (!order_pool.get_last_order().has_value())
      return SIGNAL::HOLD;

    OrdersBundle last_order = order_pool.get_last_order().value();

    // If order is not filled, the price is unavailable
    if (last_order.order_status != ORDER_STATUS::FILLED)
      return SIGNAL::HOLD;

    if (last_order.main_order.position_side == POSITION_SIDE::LONG) {
      // Stop Loss
      if (m_last_agg_trade.price <= last_order.price * (Fixed{1} - SL)) {
        order_pool.remove_order(last_order.clientOrderId);
        return SIGNAL::CLOSE_LONG;
      }

      // Take Profit
      if (m_last_agg_trade.price >= last_order.price * (Fixed{1} + TP)) {
        order_pool.remove_order(last_order.clientOrderId);
        return SIGNAL::CLOSE_LONG;
      }

      // VWAP revert
      if (delta_rel >= 0) {
        order_pool.remove_order(last_order.clientOrderId);
        return SIGNAL::CLOSE_LONG;
      }
    } else if (last_order.main_order.position_side == POSITION_SIDE::SHORT) {
      // Stop Loss
      if (m_last_agg_trade.price >= last_order.price * (Fixed{1} + SL)) {
        order_pool.remove_order(last_order.clientOrderId);
        return SIGNAL::CLOSE_SHORT;
      }

      // Take Profit
      if (m_last_agg_trade.price <= last_order.price * (Fixed{1} - TP)) {
        order_pool.remove_order(last_order.clientOrderId);
        return SIGNAL::CLOSE_SHORT;
      }

      // VWAP revert
      if (delta_rel <= 0) {
        order_pool.remove_order(last_order.clientOrderId);
        return SIGNAL::CLOSE_SHORT;
      }
    }
  }

  return SIGNAL::HOLD;
}

void MeanReversion::on_market_data(const Market::ParsedMarketData &data) {
  if (!std::holds_alternative<Market::AggTradeData>(data))
    return;

  m_last_agg_trade = std::get<Market::AggTradeData>(data);
  _update_window();
}

Fixed MeanReversion::_calculate_vwap() {
  // sum(price * volume) / sum(volume)
  return (m_agg_volume == 0) ? 0 : m_agg_price_volume / m_agg_volume;
}

void MeanReversion::_update_window() {
  m_vwap_window.push_back(m_last_agg_trade);
  m_agg_price_volume += (m_last_agg_trade.price * m_last_agg_trade.quantity);
  m_agg_volume += m_last_agg_trade.quantity;

  if (m_vwap_window.size() > VWAP_WINDOW) {
    const Market::AggTradeData &old_trade = m_vwap_window.front();
    m_agg_price_volume -= (old_trade.price * old_trade.quantity);
    m_agg_volume -= old_trade.quantity;
    m_vwap_window.pop_front();
  }
}
