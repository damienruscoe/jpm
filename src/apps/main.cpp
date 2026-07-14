#include "fixed_point.hpp"
#include "line_view.hpp"
#include "mmfile.hpp"
#include "order_book.hpp"
#include "order_id.hpp"
#include "parser/csv.hpp"
#include "render.hpp"
#include "str_utils.hpp"

#include "signals/signals.hpp"

#include <iostream>
#include <memory>

template <typename Traits> struct TradePrinter {
  void printTrade(const TradeEvent<Traits> &event) {
    std::cout << fmt::LineTag::Note("TRADED")
              << fmt::KeyValue("Order ID", event.order_id)
              << fmt::KeyValue("Quantity", event.quantity)
              << fmt::KeyValue("Price", event.price)
              << fmt::KeyValue("Aggressive Side",
                               event.aggressor_side == side_t::ASK ? 'S' : 'B');
    if (event.fill == FillStatus::Partial)
      std::cout << fmt::LineTag::Note("Partial");
    std::cout << nl;
  }
  void printFilled(const OrderMatchedEvent<Traits> &event) {
    std::cout << fmt::LineTag::Warning("FILLED")
              << fmt::KeyValue("Order ID", event.order_id)
              << fmt::KeyValue("Quantity", event.quantity)
              << fmt::KeyValue("Price", event.price)
              << fmt::KeyValue("Side", event.side == side_t::ASK ? 'S' : 'B')
              << nl;
  }
  void printResting(const OrderMatchedEvent<Traits> &event) {
    std::cout << fmt::LineTag::Debug("RESTED")
              << fmt::KeyValue("Order ID", event.order_id)
              << fmt::KeyValue("Quantity", event.quantity)
              << fmt::KeyValue("Price", event.price)
              << fmt::KeyValue("Side", event.side == side_t::ASK ? 'S' : 'B')
              << nl;
  }
  void update(const TradeEvent<Traits> &event) { printTrade(event); }
  void update(const OrderMatchedEvent<Traits> &event) {
    event.remaining == 0 ? printFilled(event) : printResting(event);
  }
};

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

using Traits = OrderBookTraits<FixedSizeOrderID, FixedPoint<4>, uint32_t>;
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

using Book = OrderBook<Traits, SignalAgregator>;

struct Venue {
  using symbol_t = decltype(parser::csv::parse_line("")->symbol);

  static void process_file(MappedFile &file, const auto &on_parsed) {
    LineView lines(reinterpret_cast<const char *>(file.data()), file.size());
    for (const auto &line : lines) {
      if (auto msg = parser::csv::parse_line(line))
        on_parsed(*msg);
      else
        std::cout << fmt::LineTag::Error("ERROR") << msg.error() << nl;
    }
  }
};

int main(int argc, char *argv[]) {
  std::string filename = argc > 1 ? argv[1] : "../docs/given_example.csv";

  MappedFile file(filename);
  if (!file.data()) {
    std::cerr << "Failed to open or map file: " << filename << nl;
    return 1;
  }

  std::unordered_map<Venue::symbol_t, Book> ticker_books;

  Venue::process_file(file, [&](const auto &msg) {
    std::cout << fmt::LineTag::Message("VALID") << msg << nl;

    auto [it, added] = ticker_books.try_emplace(msg.symbol);
    auto &book = it->second;

    parser::csv::process_csv_message(book, msg);

    // print_all_signals(book.getSignals());

    render_horizontal_orderbook(book);
    // render_vertical_orderbook(book);
    // top_of_book.render();
  });

  std::cout << nl << "<on exit>";

  for (const auto &[symbol, book] : ticker_books) {
    std::cout << "\n\033[30;47m Symbol: " << symbol << " \033[0m" << nl << nl;
    for (const auto &order : book.getOrders()) {
      const auto &order_id = order->id;
      const auto &side = order->side;
      const auto &quantity = order->quantity;
      const auto &price = order->price;

      std::cout << fmt::LineTag::Warning("ORDER")
                << fmt::KeyValue("OrderId", order_id)
                << fmt::KeyValue("Side", side == side_t::BID ? "Buy" : "Sell")
                << fmt::KeyValue("Quantity", quantity)
                << fmt::KeyValue("Price", price) << nl;
    }
    std::cout << nl;
    render_horizontal_orderbook(book);
  }

  return 0;
}
