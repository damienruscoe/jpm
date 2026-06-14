#pragma once

#include <cstdint>
#include <map>
#include <memory_resource>
#include <stdexcept>

namespace ladder {

template <typename Price, typename Quantity>
using BidBook = std::map<Price, Quantity, std::greater<Price>>;

template <typename Price, typename Quantity>
using AskBook = std::map<Price, Quantity, std::less<Price>>;

template <typename Price, typename Quantity>
using PmrBidBook = std::pmr::map<Price, Quantity, std::greater<Price>>;

template <typename Price, typename Quantity>
using PmrAskBook = std::pmr::map<Price, Quantity, std::less<Price>>;

template <typename Ladder, typename Level>
void setQuantity(Ladder& ladder, const Level& level)
{
    if (!level.quantity) {
        ladder.erase(level.price);
        return;
    }

    const auto [it, inserted] = ladder.try_emplace(level.price, level.quantity);
    if (!inserted)
        it->second = level.quantity;
}

template <typename Ladder, typename Level>
void setSpreadLimit(Ladder& ladder, const Level& level)
{
    if (level.quantity)
        ladder.erase(ladder.begin(), ladder.upper_bound(level.price));
}

template <typename Level, typename Ladder>
Level getBest(const Ladder& ladder)
{
    if (ladder.empty())
        throw std::runtime_error("Order Book Ladder: empty");
    return { ladder.begin()->first, ladder.begin()->second };
}

template <typename Level, typename Ladder>
std::vector<Level> getTop(const Ladder& ladder, uint16_t depth)
{
    std::vector<Level> result {};
    result.reserve(depth);

    for (const auto& current : ladder) {
        result.push_back({ current.first, current.second });
        if (result.size() > depth)
            break;
    }
    return result;
}

}
