#pragma once

#include <filesystem>

#include "databento/dbn_decoder.hpp"
#include "databento/record.hpp"
#include "expected.hpp"

namespace parser::databento {

std::unique_ptr<::databento::DbnDecoder>
open(const std::filesystem::path &file_path);

Expected<const ::databento::MboMsg *, std::string>
parse_message(::databento::DbnDecoder &decoder);

template <typename OrderBook>
bool update_order_book(OrderBook &book, const ::databento::MboMsg *p_mbo) {
  const auto &mbo = *p_mbo;
  bool success = false;

  switch (mbo.action) {
  case 'A':
  case 'C':
  case 'M':
    assert(mbo.side == 'A' || mbo.side == 'B');
    break;
  case 'R':
    assert(false);
    break;
  case 'T':
  case 'F':
  case 'N':
  default:
    break;
  }

  switch (mbo.action) {
  case 'A':
    success = book.newOrder(mbo.order_id,
                            mbo.side == 'A' ? OrderBook::Side::ASK
                                            : OrderBook::Side::BID,
                            OrderBook::Price::Parsed(mbo.price), mbo.size);
    break;
  case 'C':
    success = book.cancel(mbo.order_id);
    break;
  case 'M':
    success = book.amend(mbo.order_id,
                         mbo.side == 'A' ? OrderBook::Side::ASK
                                         : OrderBook::Side::BID,
                         OrderBook::Price::Parsed(mbo.price), mbo.size);
    break;
  case 'R': // Reset
  case 'T': // Trade
  case 'F': // Fill
  case 'N': // None
  default:
    break;
  }

  return success;
}

} // namespace parser::databento
