#ifndef MEAN_REVERSION_HPP
#define MEAN_REVERSION_HPP

#include "core/parsers/market_data_parser.hpp"
#include "core/utils/fixed_num.hpp"
#include "strategy.hpp"
#include <deque>

class MeanReversion : public IStrategy {
public:
  MeanReversion(const std::string &name) : IStrategy(name) {}

  SIGNAL produce_signal() override;
  void on_market_data(const Market::ParsedMarketData &data) override;

private:
  Fixed _calculate_vwap();
  void _update_window();

private:
  static constexpr int VWAP_WINDOW = 300;
  static inline const Fixed VWAP_DELTA{0.003, 3};
  // temporary store them here
  static inline const Fixed TP{0.003, 3};
  static inline const Fixed SL{0.001, 3};

  Market::AggTradeData m_last_agg_trade{};
  Fixed m_agg_volume{};
  Fixed m_agg_price_volume{};

  std::deque<Market::AggTradeData> m_vwap_window{};
};

#endif // MEAN_REVERSION_HPP
