# Project Architecture

## Overview

The C++ Exchange Simulator is a high-performance, deterministic C++17 limit order book and matching engine. The system is designed around standard C++ STL containers providing strict price-time priority matching, $O(1)$ order cancellation by OrderID, and structured trade execution reporting.

```
                  +-------------------+
                  |  MatchingEngine   |
                  +---------+---------+
                            |
                            v
                  +-------------------+
                  |     OrderBook     |
                  +---------+---------+
                            |
         +------------------+------------------+
         |                                     |
         v                                     v
+------------------+                 +------------------+
|     buyBook      |                 |     sellBook     |
| std::map<Price,  |                 | std::map<Price,  |
| std::list<Order>>|                 | std::list<Order>>|
+------------------+                 +------------------+
         |                                     |
         +------------------+------------------+
                            |
                            v
                  +-------------------+
                  |     orderMap      |
                  | std::unordered_map|
                  | <OrderID, Loc>    |
                  +-------------------+
```

---

## Core Components

### 1. `MatchingEngine` (`include/MatchingEngine.hpp`, `src/MatchingEngine.cpp`)
High-level façade wrapping the underlying `OrderBook`. Provides clean APIs for:
- `processOrder(const Order&)`
- `cancelOrder(const OrderID&)`
- `modifyOrder(const OrderID&, Price, Quantity)`
- `getTrades()` and `clearTrades()`
- `containsOrder(const OrderID&)`

### 2. `OrderBook` (`include/OrderBook.hpp`, `src/OrderBook.cpp`)
Encapsulates bid/ask books, the order-ID index, matching logic, and trade history.

### 3. `Order` (`include/order.hpp`, `include/types.hpp`)
Core data struct representing order state:
- `id` (`OrderID` / `uint64_t`)
- `traderId` (`TraderID` / `uint64_t`)
- `symbolId` (`SymbolID` / `uint32_t`)
- `side` (`Side::Buy` / `Side::Sell`)
- `price` (`Price` / `int64_t`)
- `quantity` / `remainingQuantity` (`Quantity` / `int32_t`)
- `type` (`OrderType::Limit` / `OrderType::Market`)

### 4. `Trade` (`include/Trade.hpp`)
Execution fill record:
- `tradeId` (`TradeID` / `uint64_t`)
- `makerOrderId` (`OrderID`)
- `takerOrderId` (`OrderID`)
- `price` (`Price`)
- `quantity` (`Quantity`)

### 5. `Logger` (`include/Logger.hpp`)
Single-threaded, file-based structured logger controlled via `#ifdef ENABLE_LOGGING`.

---

## Data Structures & Rationale

| Container | Purpose | Rationale |
| :--- | :--- | :--- |
| `std::map<Price, std::list<Order>, std::greater<Price>>` | `buyBook` | Keeps bids sorted in descending price order ($O(\log P)$ insertion/erasure). Highest bid is at `.begin()`. |
| `std::map<Price, std::list<Order>, std::less<Price>>` | `sellBook` | Keeps asks sorted in ascending price order ($O(\log P)$ insertion/erasure). Lowest ask is at `.begin()`. |
| `std::list<Order>` | `PriceLevel` | Implements FIFO queue per price level ($O(1)$ tail insertion, $O(1)$ head matching, iterator stability). |
| `std::unordered_map<OrderID, OrderLocation>` | `orderMap` | Index mapping order IDs to `{Side, Price, std::list::iterator}` for $O(1)$ average cancellation. |
| `std::vector<Trade>` | `trades` | Sequentially stores trade execution records ($O(1)$ amortized append). |

---

## Algorithmic Complexity

| Operation | Time Complexity | Notes |
| :--- | :--- | :--- |
| **Limit Order Insertion** | $O(\log P)$ | $P$ = number of active price levels. Tree lookup/insertion in `buyBook`/`sellBook`. |
| **Matching Execution** | $O(M)$ | $M$ = number of filled resting orders. Sequential head matching. |
| **Order Cancellation** | $O(1)$ Avg | Hash lookup in `orderMap` + $O(1)$ node erase in `std::list`. Price level map erase is $O(\log P)$ if empty. |
| **Order Modification** | $O(1)$ Cancel + $O(\log P)$ Insert | Cancels existing order and re-inserts updated order. |
| **Market Order Execution** | $O(M)$ | Matches against opposing book head nodes until filled or book exhausted; unfilled remainder discarded. |
