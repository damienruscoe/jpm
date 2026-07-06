#include "book/l2_adapter.hpp"
#include "fixed_point.hpp"
#include "line_view.hpp"
#include "mmfile.hpp"
#include "order_book.hpp"
#include "order_id.hpp"
#include "render.hpp"

#include "../src/signals/signals.hpp"

#include <functional>
#include <iostream>
#include <memory>

#include <nlohmann/json.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

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

using Traits = OrderBookTraits<FixedSizeOrderID, FixedPoint<4>, FixedPoint<4>>;
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

using Book = OrderBookL2Adapter<Traits, SignalAgregator>;

struct Message {
  side_t side;
  FixedPoint<4> quantity;
  FixedPoint<4> price;
};

struct Venue {
  using symbol_t = std::string;
  typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

  static void
  process_websocket_message(const auto &on_parsed,
                            [[maybe_unused]] websocketpp::connection_hdl hdl,
                            message_ptr msg) {
    const auto json = nlohmann::json::parse(msg->get_payload());

    /*
if (!verify_sequence_number(json))
return false;
                    */

    // uint64_t ts_ms = json.contains("timestampms") ?
    // json["timestampms"].get<uint64_t>() : 0;

    Message parsed_msg;
    for (const auto &event : json["events"]) {
      if (event["type"] == "change") {
        std::cout << event["side"].get<std::string>() << std::endl;

        parsed_msg.price =
            *FixedPoint<4>::Parse(event["price"].get<std::string>());
        parsed_msg.quantity =
            *FixedPoint<4>::Parse(event["remaining"].get<std::string>());
        parsed_msg.side = event["side"].get<std::string>()[0] == 'b'
                              ? side_t::BID
                              : side_t::ASK;

        // book.update(side, level);
        on_parsed(parsed_msg);
      }
    }
  }
};

#include <boost/asio/ssl.hpp>

typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;

context_ptr on_tls_init(websocketpp::connection_hdl hdl) {
  (void)hdl;
  context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(
      boost::asio::ssl::context::sslv23);

  try {
    ctx->set_options(boost::asio::ssl::context::default_workarounds |
                     boost::asio::ssl::context::no_sslv2 |
                     boost::asio::ssl::context::no_sslv3 |
                     boost::asio::ssl::context::single_dh_use);
  } catch (std::exception &e) {
    std::cerr << "Error in TLS context: " << e.what() << std::endl;
  }
  return ctx;
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  std::unordered_map<Venue::symbol_t, Book> ticker_books;

  const auto on_parsed = [&](const auto &msg) {
    // std::cout << VALID << msg << nl;

    auto [it, added] = ticker_books.try_emplace("btcusd");
    auto &book = it->second;

    bool success = book.setPriceLevel(msg.side, msg.price, msg.quantity);
    (void)success;

#define DEBUG 1
#ifdef DEBUG
    if (book.getBestBid() && book.getBestAsk() &&
        book.getBestBid()->price > book.getBestAsk()->price)
      std::cout << ERROR << "Crossed order book" << std::endl;
#endif

    // print_all_signals(book.getSignals());

    render_horizontal_orderbook(book);
    // render_vertical_orderbook(book);
    // top_of_book.render();
  };

  typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
  // Create a client endpoint
  client c;
  const std::string market_str = "btcusd";
  const std::string uri = "wss://api.gemini.com/v1/marketdata/" + market_str;

  try {
    // Set logging to be pretty verbose (everything except message payloads)
    c.set_access_channels(websocketpp::log::alevel::all);
    c.clear_access_channels(websocketpp::log::alevel::frame_payload);
    c.set_tls_init_handler(websocketpp::lib::bind(
        &on_tls_init, websocketpp::lib::placeholders::_1));

    // Initialize ASIO
    c.init_asio();

    // Register our message handler
    auto message_handler = [&](auto &&...args) {
      return Venue::process_websocket_message(
          on_parsed, std::forward<decltype(args)>(args)...);
    };
    c.set_message_handler(std::move(message_handler));

    websocketpp::lib::error_code ec;
    client::connection_ptr con = c.get_connection(uri, ec);
    if (ec) {
      std::cout << "could not create connection because: " << ec.message()
                << std::endl;
      return 0;
    }

    // Note that connect here only requests a connection. No network messages
    // are exchanged until the event loop starts running in the next line.
    c.connect(con);

    // Start the ASIO io_service run loop
    // this will cause a single connection to be made to the server. c.run()
    // will exit when this connection is closed.
    c.run();
  } catch (websocketpp::exception const &e) {
    std::cout << e.what() << std::endl;
  }

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
