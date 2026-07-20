#include "book/l2_adapter.hpp"
#include "core/trade_event.hpp"
#include "fixed_point.hpp"
#include "line_view.hpp"
#include "mmfile.hpp"
#include "order_book.hpp"
#include "order_id.hpp"
#include "readerwriterqueue.h"
#include "render.hpp"
#include "ui/dashboard_types.hpp"
#include "ui/root.hpp"

#include "../src/signals/signals.hpp"

#include <functional>
#include <iostream>
#include <memory>
#include <thread>

#include <nlohmann/json.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

[[maybe_unused]] constexpr std::string_view ERROR = "[\033[31mERROR\033[0m] ";

void update_level(const auto &price, const auto &quantity, auto &levels) {
  auto it = std::lower_bound(levels.begin(), levels.end(), price,
                             [](const auto &level, const auto &price) {
                               return level.price < static_cast<double>(price);
                             });

  const bool new_level =
      it == levels.end() || it->price != static_cast<double>(price);

  const double scalar = 100000.0;
  if (new_level) {
    if (quantity != 0)
      levels.insert(it, {static_cast<double>(price),
                         (uint64_t)(scalar * static_cast<double>(quantity)),
                         1,
                         {}});
  } else if (quantity == 0)
    levels.erase(it);
  else
    it->size = (scalar * static_cast<double>(quantity));
}

struct FullySequential {
  template <typename Iterator>
  static void apply(ui::OrderBookSnapshot &snapshot, const Iterator begin,
                    const Iterator end) {
    for (auto current = begin; current != end; ++current)
      update_level(current->price, current->quantity,
                   current->side == side_t::ASK ? snapshot.l2_asks
                                                : snapshot.l2_bids);
  }
};

struct SideSequential {
  template <typename Iterator>
  static void apply(ui::OrderBookSnapshot &snapshot, const Iterator begin,
                    const Iterator end) {
    const auto asks = begin;
    const auto bids = std::stable_partition(
        asks, end, [](const auto &e) { return e.side == side_t::ASK; });

    process_side(snapshot.l2_asks, asks, bids);
    process_side(snapshot.l2_bids, bids, end);
  }

private:
  static void process_side(auto &levels, const auto begin, const auto end) {
    for (auto current = begin; current != end; ++current)
      update_level(current->price, current->quantity, levels);
  }
};

struct MergeEvents {
  template <typename Iterator>
  static void apply(ui::OrderBookSnapshot &snapshot, const Iterator begin,
                    const Iterator end) {
    const auto asks = begin;
    const auto bids = std::stable_partition(
        asks, end, [](const auto &e) { return e.side == side_t::ASK; });

    process_side(snapshot.l2_asks, asks, bids);
    process_side(snapshot.l2_bids, bids, end);
  }

private:
  static void process_side(auto &levels, const auto begin, const auto end) {
    if (begin == end)
      return;

    std::stable_sort(begin, end, [](const auto &e1, const auto &e2) {
      return e1.price < e2.price;
    });

    auto current = begin;
    while (true) {
      current =
          std::adjacent_find(current, end, [](const auto &e1, const auto &e2) {
            return e1.price != e2.price;
          });
      if (current == end)
        break;

      update_level(current->price, current->quantity, levels);

      ++current;
    }
    --current;
    update_level(current->price, current->quantity, levels);
  }
};

using Traits = OrderBookTraits<FixedSizeOrderID, FixedPoint<4>, FixedPoint<4>>;
using Traits2 = OrderBookTraitsL2<Traits>::Traits;

template <typename Strategy> struct UIControllerBase {
  using Event = LevelQuantityEvent<Traits2>;
  using UIQueue = moodycamel::ReaderWriterQueue<Event>;
  UIQueue m_ui_queue{1024};

  UIControllerBase() { workload.reserve(1024); }

  void push(const Event &event) {
    bool added = m_ui_queue.enqueue(event);
    assert(added);
  }

  void process_update_queue(ui::OrderBookSnapshot &snapshot) {
    Event event;
    while (workload.size() < 1024 && m_ui_queue.try_dequeue(event))
      workload.emplace_back(std::move(event));

    snapshot.symbol = "AAPL";

    Strategy::apply(snapshot, workload.begin(), workload.end());
    workload.clear();
  }

private:
  std::vector<Event> workload;
};

using Traits = OrderBookTraits<FixedSizeOrderID, FixedPoint<4>, FixedPoint<4>>;
using Traits2 = OrderBookTraitsL2<Traits>::Traits;

// using Strategy = FullySequential;
// using Strategy = SideSequential;
using Strategy = MergeEvents;

using UIController = UIControllerBase<Strategy>;
UIController ui_controller;

template <typename Traits> struct EventHandler2 {
  void update(const TradeEvent<Traits> &event) { (void)event; }
  void update(const OrderMatchedEvent<Traits> &event) { (void)event; }
  void update(const LevelQuantityEvent<Traits> &event) {
    ui_controller.push(event);
  }
};

using Book = OrderBookL2Adapter<Traits, EventHandler2<Traits2>>;

struct Message {
  side_t side;
  FixedPoint<4> quantity;
  FixedPoint<4> price;
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

struct Gemini {
  using symbol_t = std::string;
  using File = std::pair<symbol_t, std::string>;
  using message_ptr = websocketpp::config::asio_client::message_type::ptr;
  using client = websocketpp::client<websocketpp::config::asio_tls_client>;

  static std::optional<File> open(const std::string &market_str) {
    const std::string uri = "wss://api.gemini.com/v1/marketdata/" + market_str;
    return {std::make_pair(market_str, uri)};
  }

  static symbol_t symbol(const auto msg) {
    const auto &[file, parsed_message] = msg;
    const auto &[market_str, uri] = file;
    return uri;
  }

  static void
  process_websocket_message(const auto &on_parsed, const auto &file,
                            [[maybe_unused]] websocketpp::connection_hdl hdl,
                            message_ptr msg) {
    const auto json = nlohmann::json::parse(msg->get_payload());

#ifdef VALIDATE_GEMINI
    if (!verify_sequence_number(json))
      return false;
#endif // VALIDATE_GEMINI

    // uint64_t ts_ms = json.contains("timestampms") ?
    // json["timestampms"].get<uint64_t>() : 0;

    Message parsed_msg;
    for (const auto &event : json["events"]) {
      if (event["type"] == "change") {
        // std::cout << event["side"].get<std::string>() << std::endl;

        parsed_msg.price =
            *FixedPoint<4>::Parse(event["price"].get<std::string>());
        parsed_msg.quantity =
            *FixedPoint<4>::Parse(event["remaining"].get<std::string>());
        parsed_msg.side = event["side"].get<std::string>()[0] == 'b'
                              ? side_t::BID
                              : side_t::ASK;

        auto message = std::make_pair(file, parsed_msg);
        on_parsed(message);
      }
    }
  }

  static void process_file(auto &file, const auto &on_parsed) {
    const auto &[market_str, uri] = *file;

    Gemini::client c;
    try {
      c.set_access_channels(websocketpp::log::alevel::all);
      c.clear_access_channels(websocketpp::log::alevel::frame_payload);
      c.set_tls_init_handler(websocketpp::lib::bind(
          &on_tls_init, websocketpp::lib::placeholders::_1));
      c.init_asio();

      // Register our message handler
      c.set_message_handler([&](auto &&...args) {
        return Gemini::process_websocket_message(
            on_parsed, *file, std::forward<decltype(args)>(args)...);
      });

      websocketpp::lib::error_code ec;
      Gemini::client::connection_ptr con = c.get_connection(uri, ec);
      if (ec) {
        std::cerr << "could not create connection because: " << ec.message()
                  << std::endl;
        return;
      }

      c.connect(con);
      c.run();
    } catch (websocketpp::exception const &e) {
      std::cerr << e.what() << std::endl;
    }
  }

  static void update_book(auto &book, auto &msg) {
    const auto &[file, parsed_message] = msg;

    bool success = book.setPriceLevel(parsed_message.side, parsed_message.price,
                                      parsed_message.quantity);
    (void)success;

#define DEBUG 1
#ifdef DEBUG
    if (book.getBestBid() && book.getBestAsk() &&
        book.getBestBid()->price > book.getBestAsk()->price)
      std::cout << ERROR << "Crossed order book" << std::endl;
#endif
  }
};

int main(int argc, char *argv[]) {
  std::string filename = argc > 1 ? argv[1] : "BTCUSD";

  auto ui_thread = std::jthread([&]() {
    ui::Root root;
    auto callback =
        std::bind_front(&UIController::process_update_queue, &ui_controller);
    root.update_queue = callback;
    root.run();
  });

  auto file = Gemini::open(filename);
  if (!file) {
    std::cerr << "Failed to open or map file: " << filename << nl;
    return 1;
  }

  std::unordered_map<Gemini::symbol_t, Book> ticker_books;
  Gemini::process_file(file, [&](const auto &msg) {
    auto [it, added] = ticker_books.try_emplace(Gemini::symbol(msg));
    auto &book = it->second;

    Gemini::update_book(book, msg);

    // render_horizontal_orderbook(book);
    // render_vertical_orderbook(book);
    // top_of_book.render();
  });

  return 0;
}
