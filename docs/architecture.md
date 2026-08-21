# Project Architecture

## Overview

The C++ Exchange Simulator is a high-performance, deterministic C++17 limit order book and matching engine. The system combines strict price-time priority matching with low-latency performance engineering, including a custom $O(1)$ block pool allocator and an open-addressing flat hash index.

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
+--------------------+               +--------------------+
|      buyBook       |               |      sellBook      |
|  std::map<Price,   |               |  std::map<Price,   |
|  PriceLevel>       |               |  PriceLevel>       |
|  (FixedAlloc)      |               |  (FixedAlloc)      |
+--------------------+               +--------------------+
         |                                     |
         +------------------+------------------+
                            |
                            v
                  +-------------------+
                  |     orderMap      |
                  |   FlatOrderMap    |
                  | <OrderID, Loc>    |
                  | (Open Addressing) |
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
Encapsulates bid/ask books, the order-ID index (`FlatOrderMap`), BBO fast-path matching, matching logic, and trade history.

### 3. `Order` (`include/order.hpp`, `include/types.hpp`)
Core 48-byte cache-aligned data struct representing order state:
- `id` (`OrderID` / `uint64_t`, 8 bytes)
- `traderId` (`TraderID` / `uint64_t`, 8 bytes)
- `price` (`Price` / `int64_t`, 8 bytes)
- `originalQuantity` (`Quantity` / `int32_t`, 4 bytes)
- `remainingQuantity` (`Quantity` / `int32_t`, 4 bytes)
- `symbolId` (`SymbolID` / `uint32_t`, 4 bytes)
- `side` (`Side::Buy` / `Side::Sell`, 1 byte)
- `type` (`OrderType::Limit` / `OrderType::Market`, 1 byte)
- `status` (`OrderStatus`, 1 byte)

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

## Data Structures & Performance Engineering Rationale

| Container / Type | Purpose | Low-Latency Rationale |
| :--- | :--- | :--- |
| `std::map<Price, PriceLevel, std::greater<Price>>` | `buyBook` | Keeps bids sorted in descending price order ($O(\log P)$ insertion/erasure). Highest bid is at `.begin()`. |
| `std::map<Price, PriceLevel, std::less<Price>>` | `sellBook` | Keeps asks sorted in ascending price order ($O(\log P)$ insertion/erasure). Lowest ask is at `.begin()`. |
| `std::list<Order, FixedBlockAllocator<Order>>` | `PriceLevel` | Implements FIFO queue per price level. Uses `FixedBlockAllocator` to eliminate OS heap allocation (`malloc`/`free`) overhead on order insertion/erasure. |
| `FlatOrderMap<OrderID, OrderLocation>` | `orderMap` | Open-addressing hash index with linear probing stored in a contiguous `std::vector` array. Replaces bucketed `std::unordered_map` to maximize L1/L2 cache prefetching and eliminate pointer-chasing. |
| `std::vector<Trade>` | `trades` | Sequentially stores trade execution records. Pre-allocated with `trades.reserve(10000)` to eliminate reallocation overhead. |

---

## Algorithmic Complexity

| Operation | Time Complexity | Notes |
| :--- | :--- | :--- |
| **Limit Order Insertion** | $O(\log P)$ | $P$ = number of active price levels. Tree lookup/insertion in `buyBook`/`sellBook`. |
| **Matching Execution** | $O(M)$ | $M$ = number of filled resting orders. Sequential head matching via BBO fast-path. |
| **Order Cancellation** | $O(1)$ Avg | Flat hash lookup in `FlatOrderMap` + $O(1)$ node erase in `PriceLevel`. Price level map erase is $O(\log P)$ if empty. |
| **Order Modification** | $O(1)$ In-Place / $O(\log P)$ | $O(1)$ in-place update for same-price quantity reductions (retains FIFO priority); $O(1)$ cancel + $O(\log P)$ insert otherwise. |
| **Market Order Execution** | $O(M)$ | Matches against opposing book head nodes until filled or book exhausted; unfilled remainder discarded. |

---

## System Scope & Environment Limitations

- **Single-Threaded In-Memory Engine**: Designed explicitly for high-throughput single-core matching without multi-threading locking overhead.
- **Hardware PMU Counter Limitation**: Hardware performance counter collection (`perf record`) is restricted due to sandbox container `perf_event` kernel security permissions; high-resolution standard library timers (`std::chrono::high_resolution_clock`) are used for benchmark metrics.
