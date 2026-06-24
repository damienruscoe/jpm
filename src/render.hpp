#pragma once

#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>

#include "order_book.hpp"

std::ostream &nl(std::ostream &os);

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
#if 0
  if (auto spread = book.getSpread())
    ss << "Spread: " << *spread << nl
       << "---------------------------" << nl;
#endif

  for (auto it = top_bids.cbegin(); it != top_bids.cend(); ++it)
    ss << it->price << ' ' << it->quantity << nl;

  ss << nl;
  std::cout << ss.str() << std::flush;
}

template <RenderableOrderBook OrderBook>
void render_horizontal_orderbook(const OrderBook &book, int depth = 99) {
  const auto top_asks = book.getTopAsks(depth);
  const auto top_bids = book.getTopBids(depth);

  std::stringstream ss;

  auto ask = std::make_pair(top_asks.cbegin(), top_asks.cend());
  auto bid = std::make_pair(top_bids.cbegin(), top_bids.cend());

  constexpr std::string_view FIELD_SEP = " | ";
  constexpr std::string_view SIDE_SEP = " || ";
  constexpr int WIDTH_TOTAL = 10;
  constexpr int WIDTH_QUANTITY = 10;
  constexpr int WIDTH_PRICE = 12;
  constexpr std::string_view GREEN = "\033[92m";
  constexpr std::string_view RED = "\033[91m";
  constexpr std::string_view NO_COLOUR = "\033[0m ";

  constexpr int EMPTY_LEN =
      1 + WIDTH_TOTAL + WIDTH_QUANTITY + WIDTH_PRICE + FIELD_SEP.size() * 2;

  typename OrderBook::Quantity bid_total{};
  typename OrderBook::Quantity ask_total{};

  while (ask.first != ask.second || bid.first != bid.second) {
    if (bid.first != bid.second) {
      std::stringstream price_ss;
      price_ss << bid.first->price;
      auto price = price_ss.str();

      bid_total += bid.first->quantity;

      ss << std::setfill(' ') << std::setw(WIDTH_TOTAL) << bid_total
         << FIELD_SEP << std::setfill(' ') << std::setw(WIDTH_QUANTITY)
         << bid.first->quantity << FIELD_SEP << GREEN << std::setfill(' ')
         << std::setw(WIDTH_PRICE) << price << NO_COLOUR;
      ++bid.first;
    } else
      ss << std::setfill(' ') << std::setw(EMPTY_LEN) << ' ';

    ss << SIDE_SEP << std::left;
    if (ask.first != ask.second) {
      std::stringstream price_ss;
      price_ss << ask.first->price;
      auto price = price_ss.str();

      ask_total += ask.first->quantity;

      ss << RED << std::setfill(' ') << std::setw(WIDTH_PRICE) << price
         << NO_COLOUR << FIELD_SEP << std::setfill(' ')
         << std::setw(WIDTH_QUANTITY) << ask.first->quantity << FIELD_SEP
         << std::setfill(' ') << std::setw(WIDTH_TOTAL) << ask_total;
      ++ask.first;
    } else
      ss << std::setfill(' ') << std::setw(EMPTY_LEN) << ' ';

    ss << std::right << nl;
  }

  std::cout << ss.str() << std::flush;
}
