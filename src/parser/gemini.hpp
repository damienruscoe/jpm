#pragma once

#include "core/enums.hpp"
#include "fixed_point.hpp"

#include <nlohmann/json.hpp>

namespace gemini {

struct Message {
  side_t side;
  FixedPoint<4> quantity;
  FixedPoint<4> price;
};

void parse(const std::string &msg, const auto &on_parsed) {
  const auto json = nlohmann::json::parse(msg);

#ifdef VALIDATE_GEMINI
  if (!verify_sequence_number(json))
    return false;
#endif // VALIDATE_GEMINI

  // uint64_t ts_ms = json.contains("timestampms") ?
  // json["timestampms"].get<uint64_t>() : 0;

  Message parsed_msg;
  for (const auto &event : json["events"]) {
    if (event["type"] == "change") {
      parsed_msg.price =
          *FixedPoint<4>::Parse(event["price"].template get<std::string>());
      parsed_msg.quantity =
          *FixedPoint<4>::Parse(event["remaining"].template get<std::string>());
      parsed_msg.side = event["side"].template get<std::string>()[0] == 'b'
                            ? side_t::BID
                            : side_t::ASK;

      on_parsed(parsed_msg);
    }
  }
}

} // namespace gemini
