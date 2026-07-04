#include "fixed_point.hpp"
#include "line_view.hpp"
#include "mmfile.hpp"
#include "order_book.hpp"
#include "order_id.hpp"
#include "parser/csv.hpp"
#include "render.hpp"

#include "../src/signals/signals.hpp"

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
[[maybe_unused]] constexpr std::string_view RESTING =
    "[\033[96mRESTING\033[0m] ";
[[maybe_unused]] constexpr std::string_view FILLED = "[\033[93mFILLED\033[0m] ";

template <typename Traits> struct TradePrinter {
  void printTrade(const TradeEvent<Traits> &event) {
    const char aggressor_side = event.aggressor_side == side_t::ASK ? 'S' : 'B';
    std::cout << TRADE << "Trade: " << " " << event.order_id << ", "
              << event.quantity << " " << event.price
              << ", AggrSide=" << aggressor_side
              << (event.fill == FillStatus::Partial ? " (Partial)" : "") << nl;
  }
  void printFilled(const OrderMatchedEvent<Traits> &event) {
    const char side = event.side == side_t::ASK ? 'S' : 'B';
    std::cout << FILLED << "Order ID: " << " " << event.order_id << ", "
              << event.quantity << " " << event.price << ", Side=" << side
              << nl;
  }
  void printResting(const OrderMatchedEvent<Traits> &event) {
    const char side = event.side == side_t::ASK ? 'S' : 'B';
    std::cout << RESTING << "Order ID: " << " " << event.order_id << ", "
              << event.quantity << " " << event.price << ", Side=" << side
              << nl;
  }
  void update(const TradeEvent<Traits> &event) { printTrade(event); }
  void print(const OrderMatchedEvent<Traits> &event) {
    event.remaining == 0 ? printFilled(event) : printResting(event);
  }
};

void print_all_signals(const auto &signals) {
  const int TITLE_WIDTH = 20;

  const auto print_signal = [](std::string_view name, const auto &value) {
    std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << name << ":\t"
              << FmtValueOr{value, "Unknown"} << nl;
  };

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
  std::cout << std::setfill(' ') << std::setw(TITLE_WIDTH) << "Run Length:\t"
            << signals.getRunLength() << nl;
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

void process_csv_message(Book &book, const auto &msg) {
  switch (msg.type) {
  case RequestType::New: {
    const auto side = msg.side == Side::Buy ? side_t::BID : side_t::ASK;
    auto added = book.newOrder(msg.order_id, side, msg.price, msg.quantity);
    if (!added)
      std::cout << ERROR << "Adding new order failed" << nl;
    break;
  }
  case RequestType::Cancel: {
    auto cancelled = book.cancel(msg.order_id);
    if (!cancelled)
      std::cout << ERROR << "Cancelling existing order failed" << nl;
    break;
  }
  case RequestType::AmendPriceQuantity: {
    auto amended = book.amend(msg.order_id, msg.price, msg.quantity);
    if (!amended)
      std::cout << ERROR << "Amending order failed" << nl;
    break;
  }
  default:
    break;
  }
}

struct Venue {
  using symbol_t = decltype(parse_line("")->symbol);

  static void process_file(MappedFile &file, const auto &on_parsed) {
    LineView lines(reinterpret_cast<const char *>(file.data()), file.size());
    for (const auto &line : lines) {
      if (auto msg = parse_line(line))
        on_parsed(msg);
      else
        std::cout << ERROR << msg.error() << nl;
    }
  }
};

int main(int argc, char *argv[]) {
  std::string filename = argc > 1 ? argv[1] : "docs/given_example.csv";

  MappedFile file(filename);
  if (!file.data()) {
    std::cerr << "Failed to open or map file: " << filename << nl;
    return 1;
  }

  std::unordered_map<Venue::symbol_t, Book> ticker_books;

  Venue::process_file(file, [&](const auto &msg) {
    std::cout << VALID << *msg << nl;

    auto [it, added] = ticker_books.try_emplace(msg->symbol);
    auto &book = it->second;

    process_csv_message(book, *msg);

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

      std::cout << ORDER << "OrderId: " << order_id
                << " Side: " << (side == side_t::BID ? "Buy" : "Sell")
                << " Quantity: " << quantity << " Price: " << price << nl;
    }
    std::cout << nl;
    render_horizontal_orderbook(book);
  }

  return 0;
}
