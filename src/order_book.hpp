#pragma once

#include <vector>

#include "fixed_point.hpp"
#include "ladder.hpp"

enum class side_t { BID,
    ASK };

template <typename Level>
class OrderBook {
public:
    using Price = typename Level::Price;
    using Quantity = typename Level::Quantity;

    void update(side_t side, const Level& level);

    Level getBest(side_t side) const;
    Level getBestBid() const;
    Level getBestAsk() const;

    std::vector<Level> getTop(side_t side, uint16_t depth = 10) const;
    std::vector<Level> getTopBid(uint16_t depth = 10) const;
    std::vector<Level> getTopAsk(uint16_t depth = 10) const;

private:
    using Bids = ladder::BidBook<Price, Quantity>;
    using Asks = ladder::AskBook<Price, Quantity>;

    Bids bids;
    Asks asks;
};

template <typename ActiveLadder, typename OpposingLadder, typename Level>
void update(ActiveLadder& active, OpposingLadder& opposing, const Level& level)
{
    ladder::setQuantity(active, level);
    ladder::setSpreadLimit(opposing, level);
}

template <typename Level>
void OrderBook<Level>::update(side_t side, const Level& level)
{
    side == side_t::ASK ? ::update(asks, bids, level)
                        : ::update(bids, asks, level);
}

template <typename Level>
Level OrderBook<Level>::getBest(side_t side) const
{
    return side == side_t::ASK ? getBestAsk()
                               : getBestBid();
}

template <typename Level>
std::vector<Level> OrderBook<Level>::getTop(side_t side, uint16_t depth) const
{
    return side == side_t::ASK ? getTopAsk(depth)
                               : getTopBid(depth);
}

template <typename Level>
Level OrderBook<Level>::getBestAsk() const
{
    return ladder::getBest<Level>(asks);
}

template <typename Level>
Level OrderBook<Level>::getBestBid() const
{
    return ladder::getBest<Level>(bids);
}

template <typename Level>
std::vector<Level> OrderBook<Level>::getTopAsk(uint16_t depth) const
{
    return ladder::getTop<Level>(asks, depth);
}

template <typename Level>
std::vector<Level> OrderBook<Level>::getTopBid(uint16_t depth) const
{
    return ladder::getTop<Level>(bids, depth);
}
