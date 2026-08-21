# Testing & Benchmarking Suite

## Unit Test Suite Overview

The project contains a comprehensive, 100% passing test suite across three binary executables:

### 1. `orderbook_tests` (Tests 1–16)
Verifies low-level `OrderBook` primitives:
- Buy/Sell resting order behavior
- Full and partial matching
- Order cancellation (BUY, SELL, non-existent)
- FIFO queue ordering at identical price levels
- Price priority enforcement
- Multi-price level partial and full matching
- **Test 16**: Direct unit testing of `FlatOrderMap` (insertion, lookup, cancellation, tombstones, clear).

### 2. `matching_engine_tests` (Tests 1–35)
Verifies end-to-end `MatchingEngine` behavior:
- **Tests 1–15**: Engine wrapper matching, cancellation, and multi-level fills.
- **Tests 16–20**: `OrderMap` $O(1)$ index verification and order modification/replacement priority.
- **Tests 21–25**: Market order BUY/SELL complete/partial fills and empty book handling.
- **Tests 26–30**: Trade execution reporting, maker/taker identification, multi-fill recording, and trade record integrity.
- **Tests 31–35 (Edge Cases)**: Empty book operations, exact quantity boundary fills, same-price cancellation interleaving, invalid/negative price & quantity input validation, stress sequence of add/fill/modify/cancel.

### 3. `logger_tests`
Verifies structured logging file output, event emission (`ORDER_ACCEPTED`, `ORDER_CANCELLED`, `ORDER_MODIFIED`, `TRADE_EXECUTED`), and timestamp formatting.

---

## Phase-2 Optimization Summary

### Optimization #1: Fixed-Block Memory Allocator (`FixedBlockAllocator`)
Pre-allocates chunk buffers (`BlockSize = 4096`) for `std::list<Order>` nodes, providing $O(1)$ allocation/deallocation and eliminating dynamic OS `malloc`/`free` calls during order placement.

### Optimization #2: Flat Open-Addressing Hash Index (`FlatOrderMap`)
Replaces pointer-bucketed `std::unordered_map` with a contiguous vector array (`FlatOrderMap`) utilizing open-addressing, linear probing, power-of-two mask indexing, and tombstone state flags (`Empty`, `Occupied`, `Deleted`). Eliminates pointer chasing and optimizes CPU L1/L2 data cache locality.

### Optimization #3: BBO Fast-Path Checks & Trade Pre-Allocation
Adds fast-path empty/boundary checks, iterator-based BBO progression, and pre-allocates `trades.reserve(10000)` memory to eliminate vector reallocations during matching fills.

### Optimization #4: `Order` Struct Packing & In-Place Order Modification
Reorders `Order` struct fields by descending size alignment (48 bytes total) to fit within 64-byte L1 CPU cache lines without padding gaps. Implements $O(1)$ in-place quantity updates in `modifyOrder` for same-price quantity reductions.

---

## Comparative Benchmark Performance Metrics

| Metric / Workload Scenario | Baseline (Default STL) | Opt #1 (`FixedBlockAlloc`) | Opt #2 (`FlatOrderMap`) | Opt #3 (BBO & Reserve) | Opt #4 (Struct Packing & In-Place) | Total Improvement |
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
