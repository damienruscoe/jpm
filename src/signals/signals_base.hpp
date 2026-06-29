#pragma once
#include "trade_event.hpp"

#include <functional>

namespace signals {

template <typename Traits> struct EmptySignals {
  void update(const TradeEvent<Traits> &) {}
};

template <typename OrderBook> struct BaseSignals {

  using Price = typename OrderBook::Price;
  using Quantity = typename OrderBook::Quantity;
  using L2PriceLevel = typename OrderBook::L2PriceLevel;

  // BaseSignals(const OrderBook& book) : m_book(book) {}

  /**
   * @brief Returns the spread of the best bid and best ask price.
   */
  std::optional<Price> getSpread() const {
    return BaseSignals::topOfBookInvocation(&Formulas<L2PriceLevel>::spread,
                                            m_book);
  }

  /**
   * @brief Returns the mid-price based on top-of-book (best bid/offer).
   *
   * Intuitively, this represents the "fair" value if you were to
   * instantly exit the position by trading at the market prices.
   */
  std::optional<Price> getMidPrice() const {
    return BaseSignals::topOfBookInvocation(&Formulas<L2PriceLevel>::midPrice,
                                            m_book);
  }

  /**
   * @brief Returns the volume-weighted mid-price.
   *
   * Intuitively, this gives more weight to the side of the book with more
   * depth (volume), acting as a more robust signal of price direction
   * than a simple mid-price.
   */
  std::optional<Price> getWeightedMidPrice() const {
    return BaseSignals::topOfBookInvocation(
        &Formulas<L2PriceLevel>::weightedMidPrice, m_book);
  }

  template <typename Callable>
  static auto topOfBookInvocation(Callable &&fn, const OrderBook &book) {
    auto best_bid = book.getBestBid();
    auto best_ask = book.getBestAsk();
    return (best_bid && best_ask)
               ? std::make_optional(std::invoke(std::forward<Callable>(fn),
                                                *best_bid, *best_ask))
               : std::nullopt;
  }

  /**
   * @brief Returns the micro-price derived from the inside bid/ask imbalance.
   *
   * The micro-price weights the best bid and best ask by the opposing side's
   * quantity, pulling the result toward the thinner side of the book. When
   * bid depth significantly exceeds ask depth the micro-price migrates toward
   * the ask — reflecting the higher probability that the next aggressive trade
   * will lift the offer. It is consistently a better predictor of the next
   * trade price than the simple mid-price.
   *
   * @note Algebraically equivalent to getWeightedMidPrice() but derived from
   * a distinct conceptual route (quantity imbalance ratio rather than
   * opposing-side weighting). Both are retained as separate signals because
   * they are catalogued and used independently in institutional signal
   * libraries.
   */
  std::optional<Price> getMicroPrice() const {
    return BaseSignals::topOfBookInvocation(&Formulas<L2PriceLevel>::microPrice,
                                            m_book);
  }

  /**
   * @brief Returns the normalised quantity imbalance at the inside touch.
   *
   *  ── #36 — Inside Level Volumetric Imbalance ──────────────────────────────
   *
   * Computed as (bid_qty - ask_qty) / (bid_qty + ask_qty), ranging from
   * -1 to +1. A strongly positive reading indicates that passive buy-side
   * resting depth dwarfs the offer — a bullish queue structure. A strongly
   * negative reading indicates offer-side dominance. The signal is a fast,
   * stateless proxy for short-term directional pressure and is frequently
   * used as a feature in short-horizon price prediction models alongside the
   * micro-price and spread.
   *
   * @note Unlike the micro-price (which outputs a price level), this signal
   * outputs a dimensionless ratio and should not be used as a price anchor.
   */
  std::optional<Price> getInsideVolumetricImbalance() const {
    return BaseSignals::topOfBookInvocation(
        &Formulas<L2PriceLevel>::insideVolumetricImbalance, m_book);
  }

  /**
   * @brief Returns a single-snapshot directional-pressure proxy for CVD.
   *
   * ── #49 — Cumulative Volume Delta (snapshot proxy) ───────────────────────
   *
   * @note This is NOT a true Cumulative Volume Delta. CVD is canonically a
   * running sum of signed *trade* volume over time (see
   * CumulativeVolumeDelta in trade_signals.hpp, which implements the actual
   * cumulative metric from TradeEvent prints). BaseSignals is stateless and
   * has no access to the trade stream — only to the current book snapshot —
   * so a true cumulative figure cannot be produced here.
   *
   * What this method returns instead is the signed inside-touch quantity
   * differential, (bid_qty - ask_qty), expressed in raw quantity units
   * rather than the normalised ratio used by getInsideVolumetricImbalance().
   * It is the instantaneous, non-cumulative analogue: a positive value
   * indicates more resting bid quantity than ask quantity at this instant,
   * which is directionally consistent with (but not equivalent to) a
   * positive CVD reading. Use this only as a same-tick proxy; for genuine
   * session-level buy/sell pressure tracking, use the trade-based
   * CumulativeVolumeDelta signal instead.
   */
  std::optional<Quantity> getVolumeDeltaSnapshot() const {
    return BaseSignals::topOfBookInvocation(
        &Formulas<L2PriceLevel>::volumeDeltaSnapshot, m_book);
  }

  const OrderBook &m_book;
};

} // namespace signals
