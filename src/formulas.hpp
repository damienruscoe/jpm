#pragma once

template <typename L2> struct Formulas {
  using Price = decltype(L2::price);
  using Quantity = decltype(L2::quantity);

  static Price spread(const L2 &best_bid, const L2 &best_ask) {
    return best_ask.price - best_bid.price;
  }

  static Price midPrice(const L2 &best_bid, const L2 &best_ask) {
    return (best_ask.price + best_bid.price) / Price{2};
  }

  /**
   * Weights the mid-price by opposing-side quantity so the result is pulled
   * toward whichever side carries less depth — the side that will move first
   * under marginal aggressive flow.
   *
   *   weighted_mid = (bid.price * ask.qty + ask.price * bid.qty)
   *                / (bid.qty + ask.qty)
   */
  static Price weightedMidPrice(const L2 &best_bid, const L2 &best_ask) {
    return (best_ask.price * best_bid.quantity +
            best_bid.price * best_ask.quantity) /
           (best_ask.quantity + best_bid.quantity);
  }

  /* ── #28 — Micro-Price ────────────────────────────────────────────────────
   *
   * The micro-price is the quantity-weighted mid-price expressed as a
   * function of the *imbalance ratio*:
   *
   *   I  = bid_qty / (bid_qty + ask_qty)          ∈ (0, 1)
   *   μ  = ask_price * I + bid_price * (1 - I)
   *
   * This is algebraically identical to weightedMidPrice above but is
   * derived from a different conceptual route (imbalance ratio rather than
   * opposing-side weighting) and is catalogued separately in the table
   * because it is used as a standalone signal in its own right rather than
   * as a mid-price correction.
   *
   * When bid depth >> ask depth  →  I → 1  →  μ → ask_price  (bullish lean)
   * When ask depth >> bid depth  →  I → 0  →  μ → bid_price  (bearish lean)
   */
  static auto microPrice(const L2 &bid, const L2 &ask) {
    const Price bid_qty = static_cast<Price>(bid.quantity);
    const Price ask_qty = static_cast<Price>(ask.quantity);
    const Price total_qty = bid_qty + ask_qty;
    const Price I = bid_qty / total_qty; // imbalance ratio
    return ask.price * I + bid.price * (Price{1} - I);
  }

  /* ── #36 — Inside Level Volumetric Imbalance ──────────────────────────────
   *
   * A normalised measure of directional quantity pressure at the inside touch:
   *
   *   imbal = (bid_qty - ask_qty) / (bid_qty + ask_qty)   ∈ (-1, +1)
   *
   *  +1  →  all depth is on the bid (strong buy pressure)
   *  -1  →  all depth is on the ask (strong sell pressure)
   *   0  →  perfectly balanced inside market
   *
   * This is the quantity analogue of the spread; it captures the structural
   * tilt of the inside market without requiring any rolling history.
   */
  static auto insideVolumetricImbalance(const L2 &bid, const L2 &ask) {
    const Price bid_qty = static_cast<Price>(bid.quantity);
    const Price ask_qty = static_cast<Price>(ask.quantity);
    const Price total_qty = bid_qty + ask_qty;
    return (bid_qty - ask_qty) / total_qty;
  }

  // ── #49 — Cumulative Volume Delta (snapshot proxy) ───────────────────────
  //
  // NOT a true CVD — see the doc comment on BaseSignals::getVolumeDeltaSnapshot
  // for the distinction. This returns the raw signed quantity differential at
  // the inside touch for a single snapshot:
  //
  //   delta = bid_qty - ask_qty
  //
  // Unlike insideVolumetricImbalance() this is unnormalised and returned in
  // Quantity units, not as a ratio, to mirror the unit convention of the
  // true (cumulative, trade-based) CVD signal as closely as possible.
  //
  static auto volumeDeltaSnapshot(const L2 &bid, const L2 &ask) {
    return static_cast<Quantity>(bid.quantity) -
           static_cast<Quantity>(ask.quantity);
  }
};
