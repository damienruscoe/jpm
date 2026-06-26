#pragma once

#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>

#include "order_book.hpp"
#include "str_utils.hpp"

template <typename OrderBook>
concept RenderableOrderBook = requires(const OrderBook &book) {
  book.getBestBid();
  book.getBestAsk();
};

template <RenderableOrderBook OrderBook> class TopOfBookRenderer {
public:
  TopOfBookRenderer(const OrderBook &book) : m_book(book) {}

  void render() {
    const auto bid = m_book.getBestBid();
    const auto ask = m_book.getBestAsk();
    auto new_bests = std::make_pair(bid, ask);

    if (bid && ask && new_bests != prev_bests) {
      std::cout << bid->quantity << " @ " << bid->price << "  -  "
                << ask->quantity << " @ " << ask->price << nl;
    }
  }

private:
  const OrderBook &m_book;

  using BidLevel = decltype(std::declval<OrderBook>().getBestBid());
  using AskLevel = decltype(std::declval<OrderBook>().getBestAsk());

  std::pair<BidLevel, AskLevel> prev_bests =
      std::make_pair(std::nullopt, std::nullopt);
};

template <RenderableOrderBook OrderBook>
void render_vertical_orderbook(const OrderBook &book, int depth = 99) {
  const auto top_asks = book.getTopAsks(depth);
  const auto top_bids = book.getTopBids(depth);

  std::stringstream ss;

  for (auto it = top_asks.crbegin(); it != top_asks.crend(); ++it)
    ss << it->price << ' ' << it->quantity << nl;

  ss << "---------------------------" << nl;
  if (auto spread = book.getSpread())
    ss << "Spread: " << *spread << nl << "---------------------------" << nl;

  for (auto it = top_bids.cbegin(); it != top_bids.cend(); ++it)
    ss << it->price << ' ' << it->quantity << nl;

  ss << nl;
  std::cout << ss.str() << std::flush;
}

template <RenderableOrderBook OrderBook>
void render_horizontal_orderbook(const OrderBook &book, int depth = 99) {
  constexpr std::string_view FIELD_SEP = " | ";
  constexpr std::string_view SIDE_SEP = " || ";
  constexpr int WIDTH_TOTAL = 10;
  constexpr int WIDTH_QUANTITY = 10;
  constexpr int WIDTH_PRICE = 12;
  constexpr std::string_view GREEN = "\033[92m";
  constexpr std::string_view RED = "\033[91m";
  constexpr std::string_view NO_COLOUR = "\033[0m ";

  auto left_row_cols = [&](auto &ss, const auto &total, const auto &quantity,
                           const auto &price, char fill = ' ') {
    ss << std::setfill(fill) << std::setw(WIDTH_TOTAL) << total << FIELD_SEP
       << std::setfill(fill) << std::setw(WIDTH_QUANTITY) << quantity
       << FIELD_SEP << GREEN << std::setfill(fill) << std::setw(WIDTH_PRICE)
       << price << NO_COLOUR;
  };

  auto right_row_cols = [&](auto &ss, const auto &total, const auto &quantity,
                            const auto &price, char fill = ' ') {
    ss << SIDE_SEP << std::left << RED << std::setfill(fill)
       << std::setw(WIDTH_PRICE) << price << NO_COLOUR << FIELD_SEP
       << std::setfill(fill) << std::setw(WIDTH_QUANTITY) << quantity
       << FIELD_SEP << std::setfill(fill) << std::setw(WIDTH_TOTAL) << total
       << std::right << nl;
  };

  const auto top_asks = book.getTopAsks(depth);
  const auto top_bids = book.getTopBids(depth);

  auto ask = std::make_pair(top_asks.cbegin(), top_asks.cend());
  auto bid = std::make_pair(top_bids.cbegin(), top_bids.cend());

  typename OrderBook::Quantity bid_total{};
  typename OrderBook::Quantity ask_total{};

  std::stringstream ss;

  left_row_cols(ss, "Total", "Quantity", "Price");
  right_row_cols(ss, "Total", "Quantity", "Price");
  left_row_cols(ss, "", "", "", '-');
  right_row_cols(ss, "", "", "", '-');

  while (ask.first != ask.second || bid.first != bid.second) {
    if (bid.first != bid.second) {
      std::stringstream price_ss;
      price_ss << bid.first->price;

      bid_total += bid.first->quantity;
      left_row_cols(ss, bid_total, bid.first->quantity, price_ss.str());

      ++bid.first;
    } else
      left_row_cols(ss, "", "", "");

    if (ask.first != ask.second) {
      std::stringstream price_ss;
      price_ss << ask.first->price;

      ask_total += ask.first->quantity;
      right_row_cols(ss, ask_total, ask.first->quantity, price_ss.str());

      ++ask.first;
    } else
      right_row_cols(ss, "", "", "");
  }

  std::cout << ss.str() << std::flush;
}
