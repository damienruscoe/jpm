#pragma once

#include "accumulation_distribution_line.hpp"
#include "aggressive_volume_drift.hpp"
#include "aggressive_volume_weight_drift.hpp"
#include "block_trade_detector.hpp"
#include "buy_sell_lot_ratio.hpp"
#include "chaikin_oscillator.hpp"
#include "cumulative_volume_delta.hpp"
#include "cumulative_vwap.hpp"
#include "ease_of_movement.hpp"
#include "ema_signal.hpp"
#include "flash_sweep_footprint.hpp"
#include "last_trade_price.hpp"
#include "macd_signal.hpp"
#include "money_flow_index.hpp"
#include "on_balance_volume.hpp"
#include "rsi_signal.hpp"
#include "tick_ltp_delta.hpp"
#include "tick_run_length.hpp"
#include "trade_size_depth_ratio.hpp"
#include "trade_size_zscore.hpp"
#include "volume_price_trend.hpp"
#include "volumetric_price_target_distance.hpp"
#include "vpin_signal.hpp"

#include "dynamic_composite.hpp"
#include "static_composite.hpp"

#ifdef SIGNAL_AGGREGATOR_ORIG
using SignalAgregator = signals::StaticComposite<
    Traits, TradePrinter<Traits>, signals::EmaSignal<Traits>,
    signals::LastTradePrice<Traits>, signals::CumulativeVWAP<Traits>,
    signals::TickLtpDelta<Traits>, signals::TickRunLength<Traits>,
    signals::AggressiveVolumeWeightedDrift<Traits>, signals::MacdSignal<Traits>,
    signals::RsiSignal<Traits>, signals::VolumePriceTrend<Traits>,
    signals::OnBalanceVolume<Traits>, signals::CumulativeVolumeDelta<Traits>,
    signals::VpinSignal<Traits>, signals::BlockTradeDetector<Traits>,
    signals::BuySellLotRatio<Traits>, signals::FlashSweepFootprint<Traits>,
    signals::TradeSizeZScore<Traits>, signals::TradeSizeDepthRatio<Traits>,
    signals::AggressiveVolumeDrift<Traits>, signals::MoneyFlowIndex<Traits>,
    signals::AccumulationDistributionLine<Traits>,
    signals::ChaikinOscillator<Traits>, signals::EaseOfMovement<Traits>,
    signals::VolumetricPriceTargetDistance<Traits>>;

template <typename T> void print_signal(std::string_view name, const T &value) {
  std::cout << fmt::KeyValue(name, value) << nl;
};

template <typename T>
void print_signal(std::string_view name, const std::optional<T> &value) {
  std::cout << fmt::KeyValue(name, fmt::ValueOr{value, "Unknown"}) << nl;
};

void print_all_signals(const auto &signals) {
  print_signal("LTP", signals.getLastTradedPrice());
  print_signal("MID", signals.getMidPrice());
  print_signal("MICRO", signals.getWeightedMidPrice());
  print_signal("MICRO", signals.getMicroPrice());
  print_signal("IVI", signals.getInsideVolumetricImbalance());
  print_signal("CVD (Proxy)", signals.getVolumeDeltaSnapshot());
  print_signal("EMA", signals.getEmaPrice());
  print_signal("EMA VWAP", signals.getEmaVwap());
  print_signal("Cumulative VWAP", signals.getCumulativeVwap());
  print_signal("Cumulative Volume", signals.getCumulativeVolume());
  print_signal("Cumulative Value", signals.getCumulativeValue());
  print_signal("LTP", signals.getLtpDelta());
  print_signal("Run Length", signals.getRunLength());
  print_signal("Volume Weight Drift", signals.getVolumeWeightedDrift());
  print_signal("MACD", signals.getMacd());
  print_signal("Signal Line", signals.getSignalLine());
  print_signal("RSI", signals.getRsi());
  print_signal("VPT", signals.getVpt());
  print_signal("OBV", signals.getObv());
  print_signal("CVD", signals.getCvd());
  print_signal("VPIN", signals.getVpin());
  print_signal("Blocked Trade", signals.isBlockTrade());
  print_signal("RMQ", signals.getRollingMeanQuantity());
  print_signal("BSR", signals.getBuySellRatio());
  print_signal("EMA Buy Volume", signals.getEmaBuyVolume());
  print_signal("EMA Sell Volume", signals.getEmaSellVolume());
  print_signal("Last Sweep Volume", signals.getLastSweepVolume());
  print_signal("Last Sweep Notional", signals.getLastSweepNotional());
  print_signal("Z-Score", signals.getZScore());
  print_signal("Rolling Mean", signals.getRollingMean());
  print_signal("DCR", signals.getDepthConsumptionRatio());
  print_signal("Depth Proxy", signals.getDepthProxy());
  print_signal("Volume Drift", signals.getVolumeDrift());
  print_signal("MFI", signals.getMfi());
  print_signal("Ad Line", signals.getAdLine());
  print_signal("Ad Slope", signals.getAdSlope());
  print_signal("Oscillator", signals.getOscillator());
  print_signal("EOM", signals.getEom());
  print_signal("VolumeNode", signals.getVolumeNode());
  print_signal("Price Target Distance", signals.getPriceTargetDistance());
}

#endif // SIGNAL_AGGREGATOR_ORIG
