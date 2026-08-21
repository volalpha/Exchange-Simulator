#include <iostream>
#include "MatchingEngine.hpp"

int main()
{
    MatchingEngine engine;

    Order buy1(
        1,
        101,
        1,
        Side::Buy,
        100,
        100
    );

    Order sell1(
        2,
        102,
        1,
        Side::Sell,
        105,
        40
    );

    engine.processOrder(buy1);
    engine.processOrder(sell1);

    engine.getOrderBook().print();

engine.getOrderBook().cancelOrder(2);

engine.getOrderBook().print();

  return 0;
}