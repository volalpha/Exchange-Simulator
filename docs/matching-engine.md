# Matching Engine & Order Lifecycle

## Order Lifecycle

```
[ Incoming Order ]
        |
        v
[ Input Validation ] ---> (Reject if Qty <= 0 or Limit Price <= 0)
        |
        v
[ Check Opposing Book ]
        |
        +---> [ Match Found ] ---> (Generate Trade, Decrement Qty, Erase Filled Resting Orders)
        |          |
        |          +---> (If Incoming Fully Filled -> EXIT)
        |
        +---> [ No Match / Partial Remaining ]
                   |
                   v (Limit Order)
         [ Append to Price Level (FIFO) ]
         [ Record in OrderID Index ]
```

---

## Matching Rules & Logic

### 1. Price-Time Priority
- **Price Priority**:
  - Highest Bid buys before lower bids.
  - Lowest Ask sells before higher asks.
- **Time Priority (FIFO)**:
  - Orders at the same price level execute strictly in the order they arrived (`std::list::push_back` and `level.front()`).

### 2. Execution Price Determination
- Trade execution price is always established by the **resting order's limit price** (`resting.price`).
- For both Limit vs. Limit and Market vs. Limit matches, the maker's price governs execution.

### 3. Partial Fills & Full Fills
- Executed quantity is $\text{traded} = \min(\text{incoming.remainingQuantity}, \text{resting.remainingQuantity})$.
- Quantities are decremented in-place. Fully filled resting orders ($\text{remainingQuantity} == 0$) are erased from `orderMap` and `PriceLevel`.
- Empty price levels are erased from `buyBook`/`sellBook`.

### 4. Market Orders
- Market BUY matches against lowest available SELL orders regardless of price.
- Market SELL matches against highest available BUY orders regardless of price.
- Any unfilled quantity remaining after matching against available liquidity is immediately discarded (does not rest in book or `orderMap`).

### 5. Order Cancellation & Modification
- **Cancellation**: `cancelOrder(id)` looks up `OrderLocation` in `orderMap` ($O(1)$), erases list iterator node ($O(1)$), and erases price level from map if empty.
- **Modification**: `modifyOrder(id, newPrice, newQuantity)` validates new parameters, cancels existing order (losing FIFO priority), and adds updated order.

---

## Design Tradeoffs

1. **Integer Tick Prices (`int64_t`)**: Eliminates floating-point rounding errors and precision issues during price comparison.
2. **Standard STL Primitives**: Uses `std::map`, `std::list`, and `std::unordered_map` for maximum readability, memory safety, and standard C++17 compliance.
3. **Synchronous Execution**: Single-threaded execution eliminates locking overhead and guarantees 100% deterministic test replay.

---

## Engine Limitations

- **In-Memory Storage**: Does not persist order books to disk across process restarts.
- **No Network Layer**: Operating engine API directly without network protocols (e.g. FIX, ITCH/OUCH).
- **Single Instrument per Engine**: Each `MatchingEngine` instance handles a single order book instance.
