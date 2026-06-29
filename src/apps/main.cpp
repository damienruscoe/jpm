#include "fixed_point.hpp"
#include "line_view.hpp"
#include "mmfile.hpp"
#include "order_book.hpp"
#include "order_id.hpp"
#include "parser.hpp"
#include "render.hpp"

#include "../src/signals/accumulation_distribution_line.hpp"
#include "../src/signals/aggressive_volume_drift.hpp"
#include "../src/signals/aggressive_volume_weight_drift.hpp"
#include "../src/signals/block_trade_detector.hpp"
#include "../src/signals/buy_sell_lot_ratio.hpp"
#include "../src/signals/chaikin_oscillator.hpp"
#include "../src/signals/cumulative_volume_delta.hpp"
#include "../src/signals/cumulative_vwap.hpp"
#include "../src/signals/ease_of_movement.hpp"
#include "../src/signals/ema_signal.hpp"
#include "../src/signals/flash_sweep_footprint.hpp"
#include "../src/signals/last_trade_price.hpp"
#include "../src/signals/macd_signal.hpp"
#include "../src/signals/money_flow_index.hpp"
#include "../src/signals/on_balance_volume.hpp"
#include "../src/signals/rsi_signal.hpp"
#include "../src/signals/static_composite.hpp"
#include "../src/signals/tick_ltp_delta.hpp"
#include "../src/signals/tick_run_length.hpp"
#include "../src/signals/trade_size_depth_ratio.hpp"
#include "../src/signals/trade_size_zscore.hpp"
#include "../src/signals/volume_price_trend.hpp"
#include "../src/signals/volumetric_price_target_distance.hpp"
#include "../src/signals/vpin_signal.hpp"

#include <iostream>
#include <memory>

template <typename T, bool Owned> struct FmtValueOr {
  using OptBindingRef =
      std::conditional_t<Owned, std::optional<T>, const std::optional<T> &>;

  FmtValueOr(OptBindingRef opt, std::string_view alt) : opt{opt}, alt{alt} {}

  FmtValueOr(const FmtValueOr &) = delete;
  FmtValueOr(FmtValueOr &&) noexcept = delete;
  FmtValueOr &operator=(const FmtValueOr &) = delete;
  FmtValueOr &operator=(FmtValueOr &&) = delete;

  friend std::ostream &operator<<(std::ostream &os, const FmtValueOr &&o) {
    return o.opt ? (os << *o.opt) : (os << o.alt);
  }

private:
  OptBindingRef opt;
  std::string_view alt;
};

template <typename T>
FmtValueOr(const std::optional<T> &, std::string_view) -> FmtValueOr<T, false>;

template <typename T>
FmtValueOr(std::optional<T> &&, std::string_view) -> FmtValueOr<T, true>;

[[maybe_unused]] constexpr std::string_view VALID = "[\033[32mVALID\033[0m] ";
[[maybe_unused]] constexpr std::string_view ERROR = "[\033[31mERROR\033[0m] ";
[[maybe_unused]] constexpr std::string_view TRADE = "[\033[94mTRADE\033[0m] ";
[[maybe_unused]] constexpr std::string_view ORDER = "[\033[95mORDER\033[0m] ";

using Traits = OrderBookTraits<FixedSizeOrderID, FixedPoint<4>, uint32_t>;
using SignalAgregator = signals::StaticComposite<
    Traits, signals::EmaSignal<Traits>, signals::LastTradePrice<Traits>,
    signals::CumulativeVWAP<Traits>, signals::TickLtpDelta<Traits>,
    signals::TickRunLength<Traits>,
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

using Book = OrderBook<Traits, SignalAgregator>;

int main(int argc, char *argv[]) {
  std::string filename = argc > 1 ? argv[1] : "docs/given_example.csv";

  MappedFile file(filename);
  if (!file.data()) {
    std::cerr << "Failed to open or map file: " << filename << nl;
    return 1;
  }

  LineView lines(file.data(), file.size());

  std::unordered_map<uint32_t, Book> ticker_books;

  for (const auto &line : lines) {
    if (auto msg = parse_line(line)) {
      // std::cout << VALID << *msg << nl;

      auto [it, added] = ticker_books.try_emplace(msg->exchange_ticker);
      auto &book = it->second;

      if (added) {
        book.setOnTradeCallback([](side_t side, const auto &id,
                                   const auto &price, const auto &qty,
                                   auto fill) {
          const int ticker = 101;
          const char aggressor_side = side == side_t::ASK ? 'S' : 'B';

          std::cout
              << TRADE << "Trade: " << ticker << " " << id << " " << qty << " "
              << price << ", AggrSide=" << aggressor_side
              << (fill == PriceLevel<
                              OrderBook<Traits>::Order>::FillStatus::Partial
                      ? " (Partial)"
                      : "")
              << nl;
        });
      }

      const auto side = msg->side == Side::Buy ? side_t::BID : side_t::ASK;

      switch (msg->type) {
      case RequestType::New: {
        auto added =
            book.newOrder(msg->order_id, side, msg->price, msg->quantity);
        if (!added)
          std::cout << ERROR << "Adding new order failed" << nl;
        break;
      }
      case RequestType::Cancel: {
        auto cancelled = book.cancel(msg->order_id, side);
        if (!cancelled)
          std::cout << ERROR << "Cancelling existing order failed" << nl;
        break;
      }
      case RequestType::Amend: {
        auto amended =
            book.amend(msg->order_id, side, msg->price, msg->quantity);
        if (!amended)
          std::cout << ERROR << "Amending order failed" << nl;
        break;
      }
      }

      const int TITLE_WIDTH = 20;
      const auto &signals = book.getSignals();
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "LTP:\t"
                << FmtValueOr{signals.getLastTradedPrice(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "MID:\t"
                << FmtValueOr{signals.getMidPrice(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "MICRO:\t"
                << FmtValueOr{signals.getWeightedMidPrice(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "MICRO:\t"
                << FmtValueOr{signals.getMicroPrice(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "IVI:\t"
                << FmtValueOr{signals.getInsideVolumetricImbalance(), "Unknown"}
                << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH)
                << "CVD (Proxy):\t"
                << FmtValueOr{signals.getVolumeDeltaSnapshot(), "Unknown"}
                << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "EMA:\t"
                << FmtValueOr{signals.getEmaPrice(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "EMA VWAP:\t"
                << FmtValueOr{signals.getEmaVwap(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH)
                << "Cumulative VWAP:\t"
                << FmtValueOr{signals.getCumulativeVwap(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH)
                << "Cumulative Volume:\t"
                << FmtValueOr{signals.getCumulativeVolume(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH)
                << "Cumulative Value:\t"
                << FmtValueOr{signals.getCumulativeValue(), "Unknown"} << nl;

      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "LTP:\t"
                << FmtValueOr{signals.getLtpDelta(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH)
                << "Run Length:\t" << signals.getRunLength() << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH)
                << "Volume Weight Drift:\t"
                << FmtValueOr{signals.getVolumeWeightedDrift(), "Unknown"}
                << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "MACD:\t"
                << FmtValueOr{signals.getMacd(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH)
                << "Signal Line:\t"
                << FmtValueOr{signals.getSignalLine(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "RSI:\t"
                << FmtValueOr{signals.getRsi(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "VPT:\t"
                << FmtValueOr{signals.getVpt(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "OBV:\t"
                << FmtValueOr{signals.getObv(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "CVD:\t"
                << FmtValueOr{signals.getCvd(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "VPIN:\t"
                << FmtValueOr{signals.getVpin(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH)
                << "Blocked Trade:\t"
                << FmtValueOr{signals.isBlockTrade(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "RMQ:\t"
                << FmtValueOr{signals.getRollingMeanQuantity(), "Unknown"}
                << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "BSR:\t"
                << FmtValueOr{signals.getBuySellRatio(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH)
                << "EMA Buy Volume:\t"
                << FmtValueOr{signals.getEmaBuyVolume(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH)
                << "EMA Sell Volume:\t"
                << FmtValueOr{signals.getEmaSellVolume(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH)
                << "Last Sweep Volume:\t"
                << FmtValueOr{signals.getLastSweepVolume(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH)
                << "Last Sweep Notional:\t"
                << FmtValueOr{signals.getLastSweepNotional(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "Z-Score:\t"
                << FmtValueOr{signals.getZScore(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH)
                << "Rolling Mean:\t"
                << FmtValueOr{signals.getRollingMean(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "DCR:\t"
                << FmtValueOr{signals.getDepthConsumptionRatio(), "Unknown"}
                << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH)
                << "Depth Proxy:\t"
                << FmtValueOr{signals.getDepthProxy(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH)
                << "Volume Drift:\t"
                << FmtValueOr{signals.getVolumeDrift(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "MFI:\t"
                << FmtValueOr{signals.getMfi(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "Ad Line:\t"
                << FmtValueOr{signals.getAdLine(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "Ad Slope:\t"
                << FmtValueOr{signals.getAdSlope(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH)
                << "Oscillator:\t"
                << FmtValueOr{signals.getOscillator(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "EOM:\t"
                << FmtValueOr{signals.getEom(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH)
                << "VolumeNode:\t"
                << FmtValueOr{signals.getVolumeNode(), "Unknown"} << nl;
      std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH)
                << "Price Target Distance:\t"
                << FmtValueOr{signals.getPriceTargetDistance(), "Unknown"}
                << nl;

      render_horizontal_orderbook(book);
      // render_vertical_orderbook(book);
      // top_of_book.render();
    } else
      std::cout << ERROR << msg.error() << nl;
  }

  std::cout << nl << "<on exit>";

  for (const auto &[id, book] : ticker_books) {
    std::cout << "\n\033[30;47m Ticker: " << id << " \033[0m" << nl << nl;
    for (const auto &order : book.getOrders()) {
      const auto &order_id = order->id;
      const auto &side = order->side;
      const auto &quantity = order->quantity;
      const auto &price = order->price;

      std::cout << ORDER << "OrderId: " << order_id
                << " Side: " << (side == side_t::BID ? "Buy" : "Sell")
                << " Quantity: " << quantity << " Price: " << price << nl;
    }
    std::cout << nl;
    render_horizontal_orderbook(book);
  }

  return 0;
}
