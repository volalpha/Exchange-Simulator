#pragma once
#include "types.hpp"

struct Order
{
    OrderID id;

    TraderID traderId;

    SymbolID symbolId;

    Side side;

    Price price;

    Quantity originalQuantity;

    Quantity remainingQuantity;

    OrderStatus status;

    OrderType type;

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
      symbolId(symbolId),
      side(side),
      price(price),
      originalQuantity(quantity),
      remainingQuantity(quantity),
      status(OrderStatus::Active),
      type(type)
    {
    }
};