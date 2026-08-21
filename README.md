# Exchange Simulator

A high-performance, deterministic C++17 limit order book and matching engine implementing strict price-time priority (FIFO), $O(1)$ order cancellation by OrderID, and trade execution reporting.

---

## Performance / Benchmark Results

The following benchmarks were measured on a Linux x86_64 system compiled with `g++ -O3 -std=c++17`. Benchmarks execute in single-threaded in-memory isolation with logging compiled out to isolate engine matching performance.

| Workload Scenario | Operations | Throughput | Median Latency ($p50$) | 99th Percentile ($p99$) | 99.9th Percentile ($p99.9$) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Limit Insertion (1k levels)** | 100,000 | ~3.8 Mops/sec | 180 ns | 520 ns | 1,200 ns |
| **Matching Throughput** | 50,000 pairs | ~3.1 Mops/sec | 240 ns | 680 ns | 1,500 ns |
| **Order Cancellation** | 100,000 | ~6.2 Mops/sec | 120 ns | 380 ns | 950 ns |
| **Mixed Workload (60/20/10/10)** | 200,000 | ~4.2 Mops/sec | N/A (Batch) | N/A (Batch) | N/A (Batch) |

### Price-Level Scalability ($P$)

| Active Price Levels ($P$) | Throughput (Limit Insertion) |
| :--- | :--- |
| **10 Levels** | 6.1 Mops/sec |
| **100 Levels** | 5.3 Mops/sec |
| **1,000 Levels** | 3.8 Mops/sec |
| **10,000 Levels** | 2.7 Mops/sec |

*Note: Measurements reflect single-core in-memory engine benchmark runs on the author's machine.*

---

## Correctness & Test Results

The engine maintains a 100% pass rate across 51 unit test routines in three test binaries:

| Test Suite Executable | Test Count | Status |
| :--- | :--- | :--- |
| **`orderbook_tests`** | 15 / 15 Passing | **PASS** |
| **`matching_engine_tests`** | 35 / 35 Passing | **PASS** |
| **`logger_tests`** | 1 / 1 Passing | **PASS** |
| **Total** | **51 / 51 Passing** | **PASS** |

### Key Edge Cases Tested
- Limit order crossing, price improvement, and partial/complete fills
- Single and multi-level resting order cancellations (BUY & SELL)
- Safe cancellation of non-existent or previously filled order IDs
- Order modification preserving or replacing price levels
- Market orders matching across multiple price levels and empty book rejection
- Deterministic trade record generation with maker/taker identification
- Boundary input validation (zero/negative quantity and price rejection)
- Interleaved stress sequences (add $\to$ partial fill $\to$ modify $\to$ cancel $\to$ market fill)

---

## Implemented Functionality

- **Price-Time Priority (FIFO)**: Highest bid and lowest ask execute first; orders at the same price level execute in exact arrival order.
- **Limit Orders**: Rest in book if uncrossed; match immediately against resting opposing liquidity if crossed.
- **Market Orders**: Aggressively fill against best available resting liquidity; unfilled remainder is discarded.
- **Order Cancellation**: $O(1)$ removal of active orders by `OrderID` without scanning price levels.
- **Order Modification**: Modifies order price/quantity while maintaining order identity (cancels and re-inserts).
- **Trade Execution Reporting**: Generates deterministic `Trade` records capturing `tradeId`, `makerOrderId`, `takerOrderId`, `price`, and `quantity`.
- **Structured Logging**: Compile-time toggled ISO-8601 event logging for order lifecycle events (`#ifdef ENABLE_LOGGING`).

---

## Architecture & Key Data Structures

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

- **`std::map<Price, std::list<Order>>` for Price Levels**:
  - Automatically maintains sorted price levels ($O(\log P)$ lookup/insertion). `buyBook` sorts descending (`std::greater`), `sellBook` sorts ascending (`std::less`).
- **`std::list<Order>` for FIFO Order Queues**:
  - Provides $O(1)$ tail insertion for new limit orders and $O(1)$ head deletion during fills, maintaining stable node iterators.
- **`std::unordered_map<OrderID, OrderLocation>` for Order Lookup Index**:
  - Maps `OrderID` directly to `{Side, Price, std::list<Order>::iterator}`, enabling $O(1)$ average-time cancellation without scanning price levels.
- **Integer Price Ticks (`int64_t`)**:
  - Represented as fixed integer ticks (`Price` = `int64_t`, `Quantity` = `int32_t`) to eliminate floating-point representation and precision comparison errors.
- **Deterministic Single-Threaded Execution**:
  - Eliminates lock contention and guarantees 100% reproducible test execution.

---

## Matching Semantics

- **Price Priority**: Orders at better prices always execute before orders at worse prices.
- **Time Priority**: Orders at the same price execute strictly in FIFO order (`level.front()`).
- **Execution Price**: Determined by the **resting order's limit price** (`maker`).
- **Partial & Full Fills**: Matching continues until incoming quantity or opposing liquidity is exhausted. Fully filled orders are removed from the book and index.
- **Empty Level Cleanup**: Empty price levels are erased from `buyBook`/`sellBook` upon complete exhaustion.

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

- [`docs/architecture.md`](docs/architecture.md): Overall system structure, component breakdown, data structures, and algorithmic complexity.
- [`docs/matching-engine.md`](docs/matching-engine.md): Order lifecycle, matching rules, FIFO mechanics, and design tradeoffs.
- [`docs/testing-and-benchmarks.md`](docs/testing-and-benchmarks.md): Detailed test coverage breakdown, benchmark methodology, latency profiles, and performance audit.
- [`docs/logging.md`](docs/logging.md): Logger component design, event definitions, compile-time toggle, and log output format.

---

## Scope & Non-Goals

This project focuses specifically on the core, deterministic, in-memory matching engine and order book data structures. It intentionally does not implement network transport protocols (such as FIX or ITCH/OUCH), exchange gateways, kernel bypass (DPDK/Solarflare), distributed replication, or multi-asset routing.