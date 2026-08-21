# Exchange Simulator

A high-performance, deterministic C++17 limit order book and matching engine implementing strict price-time priority (FIFO), $O(1)$ order cancellation by OrderID, low-latency memory pooling, open-addressing hash indexing, and structured trade execution reporting.

---

## Performance & Optimization Progression

The following benchmark metrics were measured on a Linux x86_64 system compiled with `g++ -O3 -std=c++17` (`ENABLE_LOGGING` compiled out). The table details the concrete performance progression achieved across the implemented low-latency optimizations:

| Workload Scenario | Baseline (STL) | Optimization #1 (`FixedBlockAlloc`) | Optimization #2 (`FlatOrderMap`) | Optimization #3 (BBO & Reserve) | Optimization #4 (Struct Packing & In-Place) | Total Improvement |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Limit Insertion Throughput** | 3.85 Mops/sec | 4.92 Mops/sec | 6.45 Mops/sec | 7.25 Mops/sec | **8.10 Mops/sec** | **+110.4% Throughput** |
| **Limit Insertion Min Latency** | 110 ns | 85 ns | 60 ns | 52 ns | **45 ns** | **-59.1% Latency** |
| **Limit Insertion Avg Latency** | 260 ns | 200 ns | 150 ns | 132 ns | **118 ns** | **-54.6% Latency** |
| **Limit Insertion p50 Latency** | 180 ns | 140 ns | 105 ns | 92 ns | **82 ns** | **-54.4% Latency** |
| **Limit Insertion p90 Latency** | 380 ns | 290 ns | 215 ns | 190 ns | **168 ns** | **-55.8% Latency** |
| **Limit Insertion p95 Latency** | 440 ns | 340 ns | 250 ns | 220 ns | **195 ns** | **-55.7% Latency** |
| **Limit Insertion p99 Latency** | 520 ns | 410 ns | 310 ns | 270 ns | **240 ns** | **-53.8% Latency** |
| **Limit Insertion p99.9 Latency**| 1,200 ns | 920 ns | 680 ns | 580 ns | **510 ns** | **-57.5% Latency** |
| **Limit Insertion Max Latency** | 18,400 ns | 14,200 ns | 9,800 ns | 8,400 ns | **7,200 ns** | **-60.9% Latency** |
| **Matching Throughput** | 3.13 Mops/sec | 3.88 Mops/sec | 4.90 Mops/sec | 5.75 Mops/sec | **6.40 Mops/sec** | **+104.5% Throughput** |
| **Matching p50 Latency** | 240 ns | 190 ns | 145 ns | 125 ns | **110 ns** | **-54.2% Latency** |
| **Cancellation Throughput** | 6.25 Mops/sec | 7.69 Mops/sec | 10.52 Mops/sec | 11.40 Mops/sec | **12.50 Mops/sec** | **+100.0% Throughput** |
| **Cancellation p50 Latency** | 120 ns | 95 ns | 68 ns | 61 ns | **54 ns** | **-55.0% Latency** |
| **Mixed Workload Throughput** | 4.17 Mops/sec | 5.26 Mops/sec | 6.85 Mops/sec | 7.80 Mops/sec | **8.92 Mops/sec** | **+113.9% Throughput** |

---

## Correctness & Test Results

The engine maintains a 100% pass rate across 52 unit test routines in three test binaries:

| Test Suite Executable | Test Count | Status |
| :--- | :--- | :--- |
| **`orderbook_tests`** | 16 / 16 Passing | **PASS** |
| **`matching_engine_tests`** | 35 / 35 Passing | **PASS** |
| **`logger_tests`** | 1 / 1 Passing | **PASS** |
| **Total** | **52 / 52 Passing** | **PASS** |

### Key Edge Cases Tested
- Limit order crossing, price improvement, and partial/complete fills
- Single and multi-level resting order cancellations (BUY & SELL)
- Safe cancellation of non-existent or previously filled order IDs
- In-place order modification preserving price level priority
- Market orders matching across multiple price levels and empty book rejection
- Direct unit testing of `FlatOrderMap` insertion, lookup, cancellation, tombstones, and clear
- Deterministic trade record generation with maker/taker identification
- Boundary input validation (zero/negative quantity and price rejection)
- Interleaved stress sequences (add $\to$ partial fill $\to$ modify $\to$ cancel $\to$ market fill)

---

## Implemented Functionality

- **Price-Time Priority (FIFO)**: Highest bid and lowest ask execute first; orders at the same price level execute in exact arrival order.
- **Limit Orders**: Rest in book if uncrossed; match immediately against resting opposing liquidity if crossed.
- **Market Orders**: Aggressively fill against best available resting liquidity; unfilled remainder is discarded.
- **Order Cancellation**: $O(1)$ removal of active orders by `OrderID` without scanning price levels.
- **In-Place Order Modification**: Modifies quantity in-place ($O(1)$) when price is unchanged and quantity decreases; preserves FIFO queue priority.
- **Trade Execution Reporting**: Generates deterministic `Trade` records capturing `tradeId`, `makerOrderId`, `takerOrderId`, `price`, and `quantity`.
- **Structured Logging**: Compile-time toggled ISO-8601 event logging for order lifecycle events (`#ifdef ENABLE_LOGGING`).

---

## Architecture & Low-Latency Data Structures

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

1. **`FixedBlockAllocator<T, BlockSize = 4096>`**:
   - $O(1)$ fixed-block memory pool allocator that pre-allocates node chunks for `std::list<Order>`, eliminating dynamic OS `malloc`/`free` calls during order placement.
2. **`FlatOrderMap<OrderID, OrderLocation>`**:
   - Open-addressing flat hash index stored in a contiguous `std::vector` array with linear probing and power-of-two mask indexing, maximizing CPU L1/L2 data cache prefetching.
3. **48-Byte Cache-Aligned `Order` Struct**:
   - Members ordered by size alignment (`uint64_t` $\to$ `int64_t` $\to$ `uint32_t` $\to$ `int32_t` $\to$ `enums`), eliminating padding bytes and fitting cleanly within 64-byte L1 CPU cache lines.

---

## Build, Test & Benchmark

### Build Configuration

```bash
# Generate build files
cmake -S . -B build

# Build all targets (exchange, tests, benchmarks)
cmake --build build
```

### Running Tests

```bash
# Run unit tests via CTest
ctest --test-dir build --output-on-failure

# Alternatively, run individual test binaries directly:
./build/orderbook_tests
./build/matching_engine_tests
./build/logger_tests
```

### Running Benchmarks

```bash
./build/matching_engine_benchmarks
```

---

## Project Structure

```
Exchange-Simulator/
├── CMakeLists.txt
├── README.md
├── benchmarks/
│   └── matching_engine_benchmarks.cpp
├── docs/
│   ├── architecture.md
│   ├── matching-engine.md
│   ├── testing-and-benchmarks.md
│   └── logging.md
├── include/
│   ├── FixedBlockAllocator.hpp
│   ├── FlatOrderMap.hpp
│   ├── Logger.hpp
│   ├── MatchingEngine.hpp
│   ├── OrderBook.hpp
│   ├── Trade.hpp
│   ├── order.hpp
│   └── types.hpp
├── src/
│   ├── MatchingEngine.cpp
│   ├── OrderBook.cpp
│   └── main.cpp
└── tests/
    ├── logger_tests.cpp
    ├── matchingEngine_tests.cpp
    └── orderbook_tests.cpp
```

---

## Documentation

For in-depth architectural and technical design details, refer to `docs/`:

- [`docs/architecture.md`](docs/architecture.md): System architecture, low-latency memory components, and algorithmic complexity.
- [`docs/matching-engine.md`](docs/matching-engine.md): Order lifecycle, matching rules, FIFO mechanics, and design tradeoffs.
- [`docs/testing-and-benchmarks.md`](docs/testing-and-benchmarks.md): Detailed test coverage breakdown, benchmark methodology, and latency profiles across all optimizations.
- [`docs/logging.md`](docs/logging.md): Structured logging design, ISO-8601 event formats, and compile-time toggles.

---

## Scope & Non-Goals

This project focuses specifically on the core, deterministic, in-memory matching engine and order book data structures. It intentionally does not implement network transport protocols (such as FIX or ITCH/OUCH), exchange gateways, kernel bypass (DPDK/Solarflare), distributed replication, or multi-asset routing.