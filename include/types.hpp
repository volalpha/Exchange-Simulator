#pragma once
#include <cstdint>

using TraderID = uint64_t;
using OrderID = uint64_t;
using Price = int64_t;
using Quantity = int32_t;
using SymbolID = uint32_t;

enum class Side{
    Buy,
    Sell
};

enum class OrderType{
    Limit,
    Market
};

enum class OrderStatus{
    Active,
    PartiallyFilled,
    Filled,
    Canceled,
    Rejected
};