#pragma once
#include "types.hpp"

using TradeID = uint64_t;

struct Trade
{
    TradeID tradeId;
    OrderID makerOrderId;
    OrderID takerOrderId;
    Price price;
    Quantity quantity;

    Trade(TradeID tradeId, OrderID makerOrderId, OrderID takerOrderId, Price price, Quantity quantity)
        : tradeId(tradeId),
          makerOrderId(makerOrderId),
          takerOrderId(takerOrderId),
          price(price),
          quantity(quantity)
    {
    }
};
