#pragma once
#include "types.hpp"

struct Order
{
    OrderID id;
    TraderID traderId;
    Price price;
    Quantity originalQuantity;
    Quantity remainingQuantity;
    SymbolID symbolId;
    Side side;
    OrderType type;
    OrderStatus status;

    Order(
        OrderID id,
        TraderID traderId,
        SymbolID symbolId,
        Side side,
        Price price,
        Quantity quantity,
        OrderType type = OrderType::Limit
    )
        : id(id),
          traderId(traderId),
          price(price),
          originalQuantity(quantity),
          remainingQuantity(quantity),
          symbolId(symbolId),
          side(side),
          type(type),
          status(OrderStatus::Active)
    {
    }
};