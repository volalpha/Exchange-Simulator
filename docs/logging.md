# Structured Logging Component

## Overview
The logging component (`include/Logger.hpp`) provides single-threaded, deterministic structured event logging for order lifecycle tracking. It is controlled via a compile-time preprocessor definition (`#ifdef ENABLE_LOGGING`), ensuring zero runtime performance overhead when logging is disabled.

---
## Log Event Types
The logger captures four primary lifecycle events:

1. **`ORDER_ACCEPTED`**: Emitted when a valid limit order is accepted into the engine.
2. **`ORDER_CANCELLED`**: Emitted when an active order is removed by OrderID.
3. **`ORDER_MODIFIED`**: Emitted when an active order's price or quantity is replaced.
4. **`TRADE_EXECUTED`**: Emitted whenever a fill occurs between a maker and taker.

---

## Compile-Time Disable (`ENABLE_LOGGING`)

- **Enabled Mode**: When `ENABLE_LOGGING` is defined (e.g. `g++ -DENABLE_LOGGING`), macro calls expand to `Logger::getInstance().log...()`.
- **Disabled Mode (Benchmark Mode)**: When `ENABLE_LOGGING` is omitted (as in `matching_engine_benchmarks`), macro wrapper definitions expand to `((void)0)` no-ops:
  ```cpp
  #ifdef ENABLE_LOGGING
      #define LOG_ORDER_ACCEPTED(id, side, price, qty) Logger::getInstance().logOrderAccepted(...)
  #else
      #define LOG_ORDER_ACCEPTED(id, side, price, qty) ((void)0)
  #endif
  ```

---
## Usage Example
```cpp
#define ENABLE_LOGGING
#include "MatchingEngine.hpp"
#include "Logger.hpp"

int main()
{
    // Open log output file
    Logger::getInstance().open("exchange.log");

    MatchingEngine engine;
    Order buy(1, 101, 1, Side::Buy, 100, 50);
    engine.processOrder(buy);

    Logger::getInstance().close();
    return 0;
}
```
---

## Sample Log Output

```text
2026-08-21T14:59:45.123Z [ORDER_ACCEPTED] OrderID=1 Side=BUY Price=100 Qty=50
2026-08-21T14:59:45.124Z [ORDER_MODIFIED] OrderID=1 NewPrice=101 NewQty=40
2026-08-21T14:59:45.125Z [TRADE_EXECUTED] TradeID=1 MakerOrderID=1 TakerOrderID=2 Price=101 Qty=20
2026-08-21T14:59:45.126Z [ORDER_CANCELLED] OrderID=1
```
