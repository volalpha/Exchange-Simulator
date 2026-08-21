# Matching Engine & Order Lifecycle

## Order Lifecycle

```
[ Incoming Order ]
        |
        v
[ Input Validation ] ---> (Reject if Qty <= 0 or Limit Price <= 0)
        |
        v
[ Check Opposing Book (BBO Fast-Path) ]
        |
        +---> [ Match Found ] ---> (Generate Trade, Decrement Qty, Erase Filled Resting Orders)
        |          |
        |          +---> (If Incoming Fully Filled -> EXIT)
        |
        +---> [ No Match / Partial Remaining ]
                   |
                   v (Limit Order)
         [ Append to Price Level (FIFO) via FixedBlockAllocator ]
         [ Record in FlatOrderMap Open-Addressing Index ]
```

---

## Matching Rules & Logic

### 1. Price-Time Priority (FIFO)
- **Price Priority**:
  - Highest Bid buys before lower bids (`buyBook`, sorted descending).
  - Lowest Ask sells before higher asks (`sellBook`, sorted ascending).
- **Time Priority (FIFO)**:
  - Orders at the same price level execute strictly in the order they arrived (`PriceLevel::push_back` and `level.front()`).

### 2. Execution Price Determination
- Trade execution price is always established by the **resting order's limit price** (`resting.price`).
- For both Limit vs. Limit and Market vs. Limit matches, the maker's price governs execution.

### 3. Partial Fills & Full Fills
- Executed quantity is $\text{traded} = \min(\text{incoming.remainingQuantity}, \text{resting.remainingQuantity})$.
- Quantities are decremented in-place. Fully filled resting orders ($\text{remainingQuantity} == 0$) are erased from `FlatOrderMap` ($O(1)$) and `PriceLevel` ($O(1)$).
- Empty price levels are erased from `buyBook`/`sellBook`.

### 4. Market Orders
- Market BUY matches against lowest available SELL orders regardless of price.
- Market SELL matches against highest available BUY orders regardless of price.
- Any unfilled quantity remaining after matching against available liquidity is immediately discarded (does not rest in book or `FlatOrderMap`).

### 5. Order Cancellation & Modification
- **Cancellation**: `cancelOrder(id)` looks up `OrderLocation` in `FlatOrderMap` ($O(1)$), erases list iterator node ($O(1)$), and erases price level from map if empty.
- **Modification**: `modifyOrder(id, newPrice, newQuantity)` checks parameter validity:
  - **In-Place Fast-Path ($O(1)$)**: If `newPrice == loc.price` and `newQuantity <= loc.iterator->remainingQuantity`, updates `remainingQuantity` in-place, preserving FIFO queue priority.
  - **Price/Quantity Re-placement**: If price changes or quantity increases, cancels the existing order and inserts the updated order (losing FIFO priority).

---

## Design Tradeoffs & Low-Latency Architecture

1. **Integer Tick Prices (`int64_t`)**: Eliminates floating-point rounding errors and precision issues during price comparison.
2. **Low-Latency Custom Allocators & Flat Indexing**: Uses `FixedBlockAllocator` ($O(1)$ node pool) and `FlatOrderMap` (open-addressing flat hash array) to eliminate heap dynamic memory allocation (`malloc`/`free`) and maximize L1/L2 cache prefetching.
3. **48-Byte Cache-Aligned `Order` Struct**: Reorders fields by size alignment (`uint64_t` $\to$ `int64_t` $\to$ `uint32_t` $\to$ `int32_t` $\to$ `enums`), fitting cleanly into 64-byte L1 CPU cache lines without padding gaps.
4. **Synchronous Single-Threaded Execution**: Single-threaded execution eliminates locking overhead and guarantees 100% deterministic test replay.

---

## Engine Limitations

- **In-Memory Storage**: Does not persist order books to disk across process restarts.
- **No Network Layer**: Operating engine API directly without network protocols (e.g. FIX, ITCH/OUCH).
- **Single Instrument per Engine**: Each `MatchingEngine` instance handles a single order book instance.
- **Environment PMU Note**: Standard Linux timer measurement (`std::chrono::high_resolution_clock`) is used; hardware performance counters (`perf record`) are restricted by Linux container `perf_event` kernel permissions.
