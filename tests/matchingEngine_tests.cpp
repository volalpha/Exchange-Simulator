#include <cassert>
#include <iostream>

#include "MatchingEngine.hpp"

int main()
{
    // TEST 1: MatchingEngine sends order to OrderBook
    {
        MatchingEngine engine;

        Order buy(
            1,
            101,
            1,
            Side::Buy,
            100,
            100
        );

        engine.processOrder(buy);

        assert(engine.getOrderBook().size() == 1);

        std::cout << "TEST 1 PASSED: Order reaches OrderBook\n";
    }

 // TEST 2: MatchingEngine processes a crossing order
{
    MatchingEngine engine;

    Order buy(
        1,
        100,
        1,
        Side::Buy,
        100,
        100
    );

    Order sell(
        2,
        100,
        1,
        Side::Sell,
        40,
        40
    );

    engine.processOrder(buy);
    engine.processOrder(sell);

    std::cout << "\nTEST 2 RESULT:\n";
    engine.getOrderBook().print();
    assert(engine.getOrderBook().size() == 1);


    std::cout << "TEST 2 PASSED: Crossing orders processed\n";
}
// ============================================================
// TEST 3: Full fill
// ============================================================
{
    MatchingEngine engine;

    Order buy(
        3,
        100,
        1,
        Side::Buy,
        100,
        100
    );

    Order sell(
        4,
        100,
        1,
        Side::Sell,
        100,
        100
    );

    engine.processOrder(buy);
    engine.processOrder(sell);

    // 100 BUY matches completely with 100 SELL.
    // Nothing should remain in the book.
    assert(engine.getOrderBook().size() == 0);

    std::cout << "TEST 3 PASSED: Full fill\n";
}


// ============================================================
// TEST 4: Partial fill
// ============================================================
{
    MatchingEngine engine;

    Order buy(
        5,
        100,
        1,
        Side::Buy,
        100,
        100
    );

    Order sell(
        6,
        100,
        1,
        Side::Sell,
        40,
        40
    );

    engine.processOrder(buy);
    engine.processOrder(sell);

    // 40 SELL matches against 40 of the BUY.
    // BUY has 60 remaining.
    assert(engine.getOrderBook().size() == 1);

    std::cout << "TEST 4 PASSED: Partial fill\n";
}


// ============================================================
// TEST 5: Non-crossing orders
// ============================================================
{
    MatchingEngine engine;

    Order buy(
        7,
        100,
        1,
        Side::Buy,
        50,
        100
    );

    Order sell(
        8,
        100,
        1,
        Side::Sell,
        105,
        100
    );

    engine.processOrder(buy);
    engine.processOrder(sell);

    // BUY 100 cannot match SELL 105.
    // Both orders remain.
    assert(engine.getOrderBook().size() == 2);

    std::cout << "TEST 5 PASSED: Non-crossing orders rest\n";
}


// ============================================================
// TEST 6: Multiple orders
// ============================================================
{
    MatchingEngine engine;

    Order buy1(
        9,
        1,
        1,
        Side::Buy,
        100,
        50
    );

    Order buy2(
        10,
        2,
        1,
        Side::Buy,
        100,
        30
    );

    Order sell(
        11,
        1,
        1,
        Side::Sell,
        100,
        60
    );
    engine.processOrder(buy1);
    engine.processOrder(buy2);
    engine.processOrder(sell);
    // FIFO:
    //
    // buy1 = 50 -> completely filled
    // sell remaining = 10
    // buy2 = 30 -> 10 gets filled
    // buy2 remaining = 20
    //
    // Therefore:
    // BUY book contains buy2 (20)
    // SELL book is empty
    assert(engine.getOrderBook().size() == 1);

    std::cout << "TEST 6 PASSED: Multiple orders matched\n";
}


// ============================================================
// TEST 7: Price priority
// ============================================================
{
    MatchingEngine engine;

    // Worse price
    Order buy1(
        12,
        1,
        1,
        Side::Buy,
        99,
        30
    );

    // Better BUY price
    Order buy2(
        13,
        2,
        1,
        Side::Buy,
        100,
        30
    );

    Order sell(
        14,
        1,
        1,
        Side::Sell,
        100,
        30
    );

    engine.processOrder(buy1);
    engine.processOrder(buy2);
    engine.processOrder(sell);

    // SELL @100 must match BUY @100 first.
    // BUY @99 does not cross SELL @100 and therefore remains.
    //
    // Expected:
    // buy2 -> completely filled
    // buy1 -> remains
    assert(engine.getOrderBook().size() == 1);

    engine.getOrderBook().print();

    std::cout << "TEST 7 PASSED: Price priority\n";
}


// ============================================================
// TEST 8: FIFO at same price
// ============================================================
{
    MatchingEngine engine;

    Order buy1(
        15,
        1,
        1,
        Side::Buy,
        100,
        30
    );

    Order buy2(
        16,
        2,
        1,
        Side::Buy,
        100,
        40
    );

    Order sell(
        17,
        1,
        1,
        Side::Sell,
        100,
        50
    );

    engine.processOrder(buy1);
    engine.processOrder(buy2);
    engine.processOrder(sell);

    // Same price -> FIFO.
    //
    // buy1 (30) comes first -> completely filled.
    // sell has 20 remaining.
    // buy2 gets 20 -> 20 remains.
    //
    // Only buy2 should remain.
    assert(engine.getOrderBook().size() == 1);

    engine.getOrderBook().print();

    // Cancel buy2.
    // If FIFO was correct, buy2 is the only remaining order.
    engine.getOrderBook().cancelOrder(16);

    assert(engine.getOrderBook().size() == 0);

    std::cout << "TEST 8 PASSED: FIFO at same price\n";
}


// ============================================================
// TEST 9: Multiple price levels
// ============================================================
{
    MatchingEngine engine;

    Order buy1(
        18,
        1,
        1,
        Side::Buy,
        100,
        30
    );

    Order buy2(
        19,
        2,
        1,
        Side::Buy,
        99,
        20
    );

    Order sell(
        20,
        1,
        1,
        Side::Sell,
        99,
        40
    );
    engine.processOrder(buy1);
    engine.processOrder(buy2);
    engine.processOrder(sell);
    // Incoming SELL @99:
    //
    // First matches best BUY @100 -> 30 filled.
    // SELL has 10 remaining.
    //
    // Then matches BUY @99 -> 10 filled.
    // BUY @99 originally had 20, so 10 remains.
    //
    // Final book:
    // BUY @99 -> Qty 10
    assert(engine.getOrderBook().size() == 1);
    engine.getOrderBook().print();
    std::cout << "TEST 9 PASSED: Multiple price levels\n";
}

// ============================================================
// TEST 10: Cancel resting BUY order
// ============================================================
{
    MatchingEngine engine;

    Order buy(21, 1, 1, Side::Buy, 100, 50);
    engine.processOrder(buy);

    assert(engine.getOrderBook().size() == 1);

    engine.cancelOrder(21);

    assert(engine.getOrderBook().size() == 0);

    std::cout << "TEST 10 PASSED: Cancel resting BUY order\n";
}

// ============================================================
// TEST 11: Cancel resting SELL order
// ============================================================
{
    MatchingEngine engine;

    Order sell(22, 1, 1, Side::Sell, 105, 40);
    engine.processOrder(sell);

    assert(engine.getOrderBook().size() == 1);

    engine.cancelOrder(22);

    assert(engine.getOrderBook().size() == 0);

    std::cout << "TEST 11 PASSED: Cancel resting SELL order\n";
}

// ============================================================
// TEST 12: Cancel one order while another remains
// ============================================================
{
    MatchingEngine engine;

    Order buy1(23, 1, 1, Side::Buy, 100, 50);
    Order buy2(24, 2, 1, Side::Buy, 100, 30);

    engine.processOrder(buy1);
    engine.processOrder(buy2);

    engine.cancelOrder(23);

    assert(engine.getOrderBook().size() == 1);

    // Verify remaining order is buy2 by canceling buy2 (ID 24) and verifying size becomes 0
    engine.cancelOrder(24);
    assert(engine.getOrderBook().size() == 0);

    std::cout << "TEST 12 PASSED: Cancel one order while another remains\n";
}

// ============================================================
// TEST 13: Cancel nonexistent order
// ============================================================
{
    MatchingEngine engine;

    Order buy(25, 1, 1, Side::Buy, 100, 50);
    engine.processOrder(buy);

    assert(engine.getOrderBook().size() == 1);

    engine.cancelOrder(9999);

    assert(engine.getOrderBook().size() == 1);

    std::cout << "TEST 13 PASSED: Cancel nonexistent order\n";
}

// ============================================================
// TEST 14: Cancel after partial fill
// ============================================================
{
    MatchingEngine engine;

    Order buy(26, 1, 1, Side::Buy, 100, 100);
    Order sell(27, 2, 1, Side::Sell, 100, 40);

    engine.processOrder(buy);
    engine.processOrder(sell);

    // 40 SELL matches against 40 of BUY. BUY (ID 26) has 60 remaining.
    assert(engine.getOrderBook().size() == 1);

    engine.cancelOrder(26);

    assert(engine.getOrderBook().size() == 0);

    std::cout << "TEST 14 PASSED: Cancel after partial fill\n";
}

// ============================================================
// TEST 15: Cancel at one price level while another price level remains
// ============================================================
{
    MatchingEngine engine;

    Order buy1(28, 1, 1, Side::Buy, 100, 50);
    Order buy2(29, 2, 1, Side::Buy, 100, 30);
    Order buy3(30, 3, 1, Side::Buy, 99, 20);

    engine.processOrder(buy1);
    engine.processOrder(buy2);
    engine.processOrder(buy3);

    // 2 price levels (100 and 99), total size is 2 price levels in map
    assert(engine.getOrderBook().size() == 2);

    // Cancel all orders at price level 100 (IDs 28 and 29)
    engine.cancelOrder(28);
    engine.cancelOrder(29);

    // Price level 100 disappears; only price level 99 remains
    assert(engine.getOrderBook().size() == 1);

    // Canceling buy3 removes remaining price level
    engine.cancelOrder(30);
    assert(engine.getOrderBook().size() == 0);

    std::cout << "TEST 15 PASSED: Cancel at one price level while another price level remains\n";
}

// ============================================================
// TEST 16: Order-ID Index Verification (a through f)
// ============================================================
{
    MatchingEngine engine;

    // a) Adding an order registers it in index
    Order buy1(31, 1, 1, Side::Buy, 100, 50);
    engine.processOrder(buy1);
    assert(engine.containsOrder(31));

    // b) Full fill removes order from index
    Order sell1(32, 2, 1, Side::Sell, 100, 50);
    engine.processOrder(sell1); // matches buy1 (31) fully
    assert(!engine.containsOrder(31)); // buy1 fully filled -> removed from index
    assert(!engine.containsOrder(32)); // sell1 fully filled -> not in index

    // c) Partial fill keeps remaining order in index, removes fully filled order
    Order buy2(33, 1, 1, Side::Buy, 100, 100);
    engine.processOrder(buy2);
    assert(engine.containsOrder(33));

    Order sell2(34, 2, 1, Side::Sell, 100, 40);
    engine.processOrder(sell2); // sell2 fully filled (40), buy2 partially filled (60 remaining)
    assert(engine.containsOrder(33));  // buy2 still in index with remaining quantity
    assert(!engine.containsOrder(34)); // sell2 fully filled -> not in index

    // d) Cancellation removes order from index
    engine.cancelOrder(33);
    assert(!engine.containsOrder(33)); // cancelled buy2 -> removed from index

    // e) Cancellation of nonexistent ID is safe and does not corrupt index
    Order buy3(35, 1, 1, Side::Buy, 99, 30);
    engine.processOrder(buy3);
    assert(engine.containsOrder(35));
    engine.cancelOrder(9999); // nonexistent ID
    assert(engine.containsOrder(35)); // buy3 still in index
    assert(!engine.containsOrder(9999));

    // f) Cancellation after another order at the same price remains
    Order buy4(36, 1, 1, Side::Buy, 100, 40);
    Order buy5(37, 2, 1, Side::Buy, 100, 20);
    engine.processOrder(buy4);
    engine.processOrder(buy5);
    assert(engine.containsOrder(36));
    assert(engine.containsOrder(37));

    engine.cancelOrder(36); // cancel first order at price 100
    assert(!engine.containsOrder(36)); // cancelled order removed from index
    assert(engine.containsOrder(37));  // second order at price 100 remains in index

    std::cout << "TEST 16 PASSED: Order-ID Index correctness verified (a-f)\n";
}

// ============================================================
// TEST 17: Modify quantity
// ============================================================
{
    MatchingEngine engine;

    Order buy(40, 1, 1, Side::Buy, 100, 50);
    engine.processOrder(buy);
    assert(engine.containsOrder(40));

    bool modified = engine.modifyOrder(40, 100, 80);
    assert(modified);
    assert(engine.containsOrder(40));
    assert(engine.getOrderBook().size() == 1);

    // Verify quantity updated to 80 by matching a sell order of 80 @ 100
    Order sell(41, 2, 1, Side::Sell, 100, 80);
    engine.processOrder(sell);

    assert(!engine.containsOrder(40));
    assert(!engine.containsOrder(41));
    assert(engine.getOrderBook().empty());

    std::cout << "TEST 17 PASSED: Modify quantity\n";
}

// ============================================================
// TEST 18: Modify price
// ============================================================
{
    MatchingEngine engine;

    Order buy(42, 1, 1, Side::Buy, 100, 50);
    engine.processOrder(buy);
    assert(engine.containsOrder(42));
    assert(engine.getOrderBook().size() == 1);

    bool modified = engine.modifyOrder(42, 105, 50);
    assert(modified);
    assert(engine.containsOrder(42));
    assert(engine.getOrderBook().size() == 1);

    // Match sell @ 105 to verify order appeared at new price 105 and old price level 100 was removed
    Order sell(43, 2, 1, Side::Sell, 105, 50);
    engine.processOrder(sell);

    assert(!engine.containsOrder(42));
    assert(engine.getOrderBook().empty());

    std::cout << "TEST 18 PASSED: Modify price\n";
}

// ============================================================
// TEST 19: Replace order / FIFO priority
// ============================================================
{
    MatchingEngine engine;

    Order buy1(44, 1, 1, Side::Buy, 100, 30);
    Order buy2(45, 2, 1, Side::Buy, 100, 30);
    engine.processOrder(buy1);
    engine.processOrder(buy2);

    // Modify buy1 -> loses FIFO priority and goes behind buy2
    bool modified = engine.modifyOrder(44, 100, 40);
    assert(modified);
    assert(engine.containsOrder(44));
    assert(engine.containsOrder(45));

    // Process sell of 30 @ 100 -> must match buy2 (45) first due to priority
    Order sell(46, 3, 1, Side::Sell, 100, 30);
    engine.processOrder(sell);

    assert(!engine.containsOrder(45)); // buy2 fully matched
    assert(engine.containsOrder(44));  // replaced buy1 remains
    assert(engine.getOrderBook().size() == 1);

    std::cout << "TEST 19 PASSED: Replace order / FIFO priority\n";
}

// ============================================================
// TEST 20: Modify/cancel edge cases
// ============================================================
{
    MatchingEngine engine;

    // 1) Modify nonexistent order
    assert(!engine.modifyOrder(9999, 100, 50));

    // 2) Modify already fully filled order
    Order buy1(47, 1, 1, Side::Buy, 100, 50);
    Order sell1(48, 2, 1, Side::Sell, 100, 50);
    engine.processOrder(buy1);
    engine.processOrder(sell1);
    assert(!engine.containsOrder(47));
    assert(!engine.modifyOrder(47, 100, 60));

    // 3) Modify already cancelled order
    Order buy2(49, 1, 1, Side::Buy, 100, 50);
    engine.processOrder(buy2);
    engine.cancelOrder(49);
    assert(!engine.containsOrder(49));
    assert(!engine.modifyOrder(49, 100, 60));

    // 4) Invalid quantity (<= 0)
    Order buy3(50, 1, 1, Side::Buy, 100, 50);
    engine.processOrder(buy3);
    assert(!engine.modifyOrder(50, 100, 0));
    assert(!engine.modifyOrder(50, 100, -10));

    // 5) Invalid price (<= 0)
    assert(!engine.modifyOrder(50, 0, 50));
    assert(!engine.modifyOrder(50, -100, 50));

    // Verify buy3 (50) is uncorrupted and intact
    assert(engine.containsOrder(50));
    assert(engine.getOrderBook().size() == 1);
    engine.cancelOrder(50);
    assert(engine.getOrderBook().empty());

    std::cout << "TEST 20 PASSED: Modify/cancel edge cases\n";
}

// ============================================================
// TEST 21: Market BUY complete fill
// ============================================================
{
    MatchingEngine engine;

    Order sell(51, 1, 1, Side::Sell, 100, 50);
    engine.processOrder(sell);
    assert(engine.containsOrder(51));

    Order mktBuy(52, 2, 1, Side::Buy, 0, 50, OrderType::Market);
    engine.processOrder(mktBuy);

    assert(!engine.containsOrder(51)); // SELL order fully consumed
    assert(!engine.containsOrder(52)); // Market BUY does not rest
    assert(engine.getOrderBook().empty()); // Price level cleaned up

    std::cout << "TEST 21 PASSED: Market BUY complete fill\n";
}

// ============================================================
// TEST 22: Market SELL complete fill
// ============================================================
{
    MatchingEngine engine;

    Order buy(53, 1, 1, Side::Buy, 100, 50);
    engine.processOrder(buy);
    assert(engine.containsOrder(53));

    Order mktSell(54, 2, 1, Side::Sell, 0, 50, OrderType::Market);
    engine.processOrder(mktSell);

    assert(!engine.containsOrder(53)); // BUY order fully consumed
    assert(!engine.containsOrder(54)); // Market SELL does not rest
    assert(engine.getOrderBook().empty()); // Price level cleaned up

    std::cout << "TEST 22 PASSED: Market SELL complete fill\n";
}

// ============================================================
// TEST 23: Market BUY across multiple price levels
// ============================================================
{
    MatchingEngine engine;

    Order sell1(55, 1, 1, Side::Sell, 100, 30);
    Order sell2(56, 2, 1, Side::Sell, 101, 40);
    Order sell3(57, 3, 1, Side::Sell, 102, 50);
    engine.processOrder(sell1);
    engine.processOrder(sell2);
    engine.processOrder(sell3);

    Order mktBuy(58, 4, 1, Side::Buy, 0, 100, OrderType::Market);
    engine.processOrder(mktBuy);

    assert(!engine.containsOrder(55)); // sell @ 100 fully consumed
    assert(!engine.containsOrder(56)); // sell @ 101 fully consumed
    assert(engine.containsOrder(57));  // sell @ 102 partially consumed (20 remaining)
    assert(!engine.containsOrder(58)); // Market BUY does not rest
    assert(engine.getOrderBook().size() == 1); // Only price level 102 remains

    // Match remaining 20 @ 102 to verify quantity
    Order buy(59, 5, 1, Side::Buy, 102, 20);
    engine.processOrder(buy);
    assert(!engine.containsOrder(57));
    assert(engine.getOrderBook().empty());

    std::cout << "TEST 23 PASSED: Market BUY across multiple price levels\n";
}

// ============================================================
// TEST 24: Market SELL across multiple price levels
// ============================================================
{
    MatchingEngine engine;

    Order buy1(60, 1, 1, Side::Buy, 100, 30);
    Order buy2(61, 2, 1, Side::Buy, 99, 40);
    Order buy3(62, 3, 1, Side::Buy, 98, 50);
    engine.processOrder(buy1);
    engine.processOrder(buy2);
    engine.processOrder(buy3);

    Order mktSell(63, 4, 1, Side::Sell, 0, 100, OrderType::Market);
    engine.processOrder(mktSell);

    assert(!engine.containsOrder(60)); // buy @ 100 fully consumed
    assert(!engine.containsOrder(61)); // buy @ 99 fully consumed
    assert(engine.containsOrder(62));  // buy @ 98 partially consumed (20 remaining)
    assert(!engine.containsOrder(63)); // Market SELL does not rest
    assert(engine.getOrderBook().size() == 1); // Only price level 98 remains

    // Match remaining 20 @ 98 to verify quantity
    Order sell(64, 5, 1, Side::Sell, 98, 20);
    engine.processOrder(sell);
    assert(!engine.containsOrder(62));
    assert(engine.getOrderBook().empty());

    std::cout << "TEST 24 PASSED: Market SELL across multiple price levels\n";
}

// ============================================================
// TEST 25: Market order insufficient liquidity / empty book
// ============================================================
{
    MatchingEngine engine;

    // A) Empty book case: Market BUY with no SELL liquidity
    Order mktBuyEmpty(65, 1, 1, Side::Buy, 0, 100, OrderType::Market);
    engine.processOrder(mktBuyEmpty);
    assert(engine.getOrderBook().empty());
    assert(!engine.containsOrder(65));

    // Empty book case: Market SELL with no BUY liquidity
    Order mktSellEmpty(66, 2, 1, Side::Sell, 0, 100, OrderType::Market);
    engine.processOrder(mktSellEmpty);
    assert(engine.getOrderBook().empty());
    assert(!engine.containsOrder(66));

    // B) Insufficient liquidity case: Market BUY Qty 100 vs SELL Qty 60
    Order sell1(67, 3, 1, Side::Sell, 100, 60);
    engine.processOrder(sell1);
    Order mktBuyPartial(68, 4, 1, Side::Buy, 0, 100, OrderType::Market);
    engine.processOrder(mktBuyPartial);

    assert(!engine.containsOrder(67)); // sell1 fully consumed
    assert(!engine.containsOrder(68)); // remaining 40 of market buy discarded (does not rest)
    assert(engine.getOrderBook().empty());

    // C) Insufficient liquidity case: Market SELL Qty 100 vs BUY Qty 60
    Order buy1(69, 5, 1, Side::Buy, 100, 60);
    engine.processOrder(buy1);
    Order mktSellPartial(70, 6, 1, Side::Sell, 0, 100, OrderType::Market);
    engine.processOrder(mktSellPartial);

    assert(!engine.containsOrder(69)); // buy1 fully consumed
    assert(!engine.containsOrder(70)); // remaining 40 of market sell discarded (does not rest)
    assert(engine.getOrderBook().empty());

    // D) Zero / Invalid quantity market order
    Order mktZero(71, 7, 1, Side::Buy, 0, 0, OrderType::Market);
    engine.processOrder(mktZero);
    assert(engine.getOrderBook().empty());
    assert(!engine.containsOrder(71));

    std::cout << "TEST 25 PASSED: Market order insufficient liquidity / empty book\n";
}

// ============================================================
// TEST 26: Single full trade execution
// ============================================================
{
    MatchingEngine engine;

    Order buy(72, 1, 1, Side::Buy, 100, 50);
    Order sell(73, 2, 1, Side::Sell, 100, 50);

    engine.processOrder(buy);
    engine.processOrder(sell);

    const auto& trades = engine.getTrades();
    assert(trades.size() == 1);
    assert(trades[0].quantity == 50);
    assert(trades[0].price == 100);
    assert(trades[0].makerOrderId == 72);
    assert(trades[0].takerOrderId == 73);

    assert(!engine.containsOrder(72));
    assert(!engine.containsOrder(73));
    assert(engine.getOrderBook().empty());

    std::cout << "TEST 26 PASSED: Single full trade execution\n";
}

// ============================================================
// TEST 27: Partial fill trade execution
// ============================================================
{
    MatchingEngine engine;

    Order buy(74, 1, 1, Side::Buy, 100, 100);
    Order sell(75, 2, 1, Side::Sell, 100, 40);

    engine.processOrder(buy);
    engine.processOrder(sell);

    const auto& trades = engine.getTrades();
    assert(trades.size() == 1);
    assert(trades[0].quantity == 40);
    assert(trades[0].price == 100);
    assert(trades[0].makerOrderId == 74);
    assert(trades[0].takerOrderId == 75);

    assert(engine.containsOrder(74));  // BUY remains with 60
    assert(!engine.containsOrder(75)); // SELL fully filled
    assert(engine.getOrderBook().size() == 1);

    std::cout << "TEST 27 PASSED: Partial fill trade execution\n";
}

// ============================================================
// TEST 28: Multiple trade executions
// ============================================================
{
    MatchingEngine engine;

    Order buy1(76, 1, 1, Side::Buy, 100, 50);
    Order buy2(77, 2, 1, Side::Buy, 100, 30);
    engine.processOrder(buy1);
    engine.processOrder(buy2);

    Order sell(78, 3, 1, Side::Sell, 100, 70);
    engine.processOrder(sell);

    const auto& trades = engine.getTrades();
    assert(trades.size() == 2);

    // First trade consumes buy1 (76) for 50
    assert(trades[0].makerOrderId == 76);
    assert(trades[0].takerOrderId == 78);
    assert(trades[0].price == 100);
    assert(trades[0].quantity == 50);

    // Second trade consumes buy2 (77) for 20
    assert(trades[1].makerOrderId == 77);
    assert(trades[1].takerOrderId == 78);
    assert(trades[1].price == 100);
    assert(trades[1].quantity == 20);

    assert(!engine.containsOrder(76));
    assert(engine.containsOrder(77));  // buy2 has 10 remaining
    assert(!engine.containsOrder(78));
    assert(engine.getOrderBook().size() == 1);

    std::cout << "TEST 28 PASSED: Multiple trade executions\n";
}

// ============================================================
// TEST 29: Multi-price trade execution
// ============================================================
{
    MatchingEngine engine;

    Order buy1(79, 1, 1, Side::Buy, 100, 30);
    Order buy2(80, 2, 1, Side::Buy, 99, 40);
    engine.processOrder(buy1);
    engine.processOrder(buy2);

    Order sell(81, 3, 1, Side::Sell, 99, 50);
    engine.processOrder(sell);

    const auto& trades = engine.getTrades();
    assert(trades.size() == 2);

    // First execution at price 100 for Qty 30
    assert(trades[0].makerOrderId == 79);
    assert(trades[0].takerOrderId == 81);
    assert(trades[0].price == 100);
    assert(trades[0].quantity == 30);

    // Second execution at price 99 for Qty 20
    assert(trades[1].makerOrderId == 80);
    assert(trades[1].takerOrderId == 81);
    assert(trades[1].price == 99);
    assert(trades[1].quantity == 20);

    assert(!engine.containsOrder(79));
    assert(engine.containsOrder(80)); // buy2 has 20 remaining @ 99
    assert(!engine.containsOrder(81));
    assert(engine.getOrderBook().size() == 1);

    std::cout << "TEST 29 PASSED: Multi-price trade execution\n";
}

// ============================================================
// TEST 30: Trade record integrity
// ============================================================
{
    MatchingEngine engine;

    // 1) Setup Limit orders on Sell side across 2 price levels
    Order sell1(82, 1, 1, Side::Sell, 100, 30);
    Order sell2(83, 2, 1, Side::Sell, 101, 50);
    engine.processOrder(sell1);
    engine.processOrder(sell2);

    // 2) Process Market BUY Qty 60 (consumes 30 @ 100, 30 @ 101)
    Order mktBuy(84, 3, 1, Side::Buy, 0, 60, OrderType::Market);
    engine.processOrder(mktBuy);

    const auto& trades = engine.getTrades();
    assert(trades.size() == 2);

    // Verify trade IDs are unique and strictly increasing
    assert(trades[0].tradeId < trades[1].tradeId);

    // Trade 1 integrity check
    assert(trades[0].tradeId > 0);
    assert(trades[0].makerOrderId == 82);
    assert(trades[0].takerOrderId == 84);
    assert(trades[0].price == 100);
    assert(trades[0].quantity == 30);

    // Trade 2 integrity check
    assert(trades[1].tradeId > 0);
    assert(trades[1].makerOrderId == 83);
    assert(trades[1].takerOrderId == 84);
    assert(trades[1].price == 101);
    assert(trades[1].quantity == 30);

    // Order state integrity check
    assert(!engine.containsOrder(82));
    assert(engine.containsOrder(83));  // sell2 has 20 remaining @ 101
    assert(!engine.containsOrder(84));

    std::cout << "TEST 30 PASSED: Trade record integrity\n";
}

// ============================================================
// TEST 31: Empty book / no liquidity behavior
// ============================================================
{
    MatchingEngine engine;

    // Verify initial empty state
    assert(engine.getOrderBook().empty());
    assert(engine.getOrderBook().size() == 0);
    assert(engine.getTrades().empty());
    assert(!engine.containsOrder(100));

    // Cancel on empty book
    engine.cancelOrder(100);
    assert(engine.getOrderBook().empty());

    // Modify on empty book
    assert(!engine.modifyOrder(100, 105, 10));
    assert(engine.getOrderBook().empty());

    // Market buy on empty book
    Order mktBuy(101, 1, 1, Side::Buy, 0, 50, OrderType::Market);
    engine.processOrder(mktBuy);
    assert(engine.getOrderBook().empty());
    assert(engine.getTrades().empty());
    assert(!engine.containsOrder(101));

    // Market sell on empty book
    Order mktSell(102, 2, 1, Side::Sell, 0, 50, OrderType::Market);
    engine.processOrder(mktSell);
    assert(engine.getOrderBook().empty());
    assert(engine.getTrades().empty());
    assert(!engine.containsOrder(102));

    std::cout << "TEST 31 PASSED: Empty book / no liquidity behavior\n";
}

// ============================================================
// TEST 32: Exact quantity / boundary matching behavior
// ============================================================
{
    MatchingEngine engine;

    Order sell1(103, 1, 1, Side::Sell, 100, 30);
    Order sell2(104, 2, 1, Side::Sell, 100, 20);
    Order sell3(105, 3, 1, Side::Sell, 101, 50);
    engine.processOrder(sell1);
    engine.processOrder(sell2);
    engine.processOrder(sell3);

    assert(engine.getOrderBook().size() == 2);
    assert(engine.containsOrder(103));
    assert(engine.containsOrder(104));
    assert(engine.containsOrder(105));

    // Incoming BUY exactly equals total available liquidity (30 + 20 + 50 = 100)
    Order buy(106, 4, 1, Side::Buy, 101, 100);
    engine.processOrder(buy);

    // Verify all orders are completely filled and erased from orderMap
    assert(!engine.containsOrder(103));
    assert(!engine.containsOrder(104));
    assert(!engine.containsOrder(105));
    assert(!engine.containsOrder(106));

    // Verify order book is completely empty and price levels 100 and 101 are erased
    assert(engine.getOrderBook().empty());
    assert(engine.getOrderBook().size() == 0);

    // Verify exactly 3 trade records were generated
    const auto& trades = engine.getTrades();
    assert(trades.size() == 3);
    assert(trades[0].quantity == 30 && trades[0].price == 100);
    assert(trades[1].quantity == 20 && trades[1].price == 100);
    assert(trades[2].quantity == 50 && trades[2].price == 101);

    std::cout << "TEST 32 PASSED: Exact quantity / boundary matching behavior\n";
}

// ============================================================
// TEST 33: Multiple orders at same price with cancellation/interleaving
// ============================================================
{
    MatchingEngine engine;

    Order buy1(107, 1, 1, Side::Buy, 100, 10);
    Order buy2(108, 2, 1, Side::Buy, 100, 20);
    Order buy3(109, 3, 1, Side::Buy, 100, 30);
    engine.processOrder(buy1);
    engine.processOrder(buy2);
    engine.processOrder(buy3);

    assert(engine.containsOrder(107));
    assert(engine.containsOrder(108));
    assert(engine.containsOrder(109));

    // Cancel middle order buy2 (108)
    engine.cancelOrder(108);
    assert(!engine.containsOrder(108));
    assert(engine.containsOrder(107));
    assert(engine.containsOrder(109));

    // Match sell Qty 15 @ 100 -> consumes buy1 (10) fully, and consumes 5 from buy3
    Order sell1(110, 4, 1, Side::Sell, 100, 15);
    engine.processOrder(sell1);

    assert(!engine.containsOrder(107)); // buy1 fully consumed
    assert(engine.containsOrder(109));  // buy3 remains with 25
    assert(engine.getOrderBook().size() == 1);

    // Add buy4 (111) @ 100 Qty 15 behind buy3
    Order buy4(111, 5, 1, Side::Buy, 100, 15);
    engine.processOrder(buy4);

    // Cancel buy3 (109)
    engine.cancelOrder(109);
    assert(!engine.containsOrder(109));
    assert(engine.containsOrder(111));

    // Match sell Qty 15 @ 100 against buy4 (111)
    Order sell2(112, 6, 1, Side::Sell, 100, 15);
    engine.processOrder(sell2);

    assert(!engine.containsOrder(111));
    assert(engine.getOrderBook().empty()); // Price level 100 fully erased

    std::cout << "TEST 33 PASSED: Multiple orders at same price with cancellation/interleaving\n";
}

// ============================================================
// TEST 34: Invalid or boundary order inputs
// ============================================================
{
    MatchingEngine engine;

    // Add valid order buy1 (113)
    Order buy1(113, 1, 1, Side::Buy, 100, 50);
    engine.processOrder(buy1);
    assert(engine.containsOrder(113));
    assert(engine.getOrderBook().size() == 1);

    // Invalid orders (should be rejected/ignored without corrupting engine state)
    Order invalidQtyZero(114, 2, 1, Side::Buy, 100, 0);
    engine.processOrder(invalidQtyZero);
    assert(!engine.containsOrder(114));

    Order invalidQtyNeg(115, 3, 1, Side::Buy, 100, -20);
    engine.processOrder(invalidQtyNeg);
    assert(!engine.containsOrder(115));

    Order invalidPriceZero(116, 4, 1, Side::Buy, 0, 50);
    engine.processOrder(invalidPriceZero);
    assert(!engine.containsOrder(116));

    Order invalidPriceNeg(117, 5, 1, Side::Buy, -100, 50);
    engine.processOrder(invalidPriceNeg);
    assert(!engine.containsOrder(117));

    // Cancel boundary/nonexistent IDs
    engine.cancelOrder(0);
    engine.cancelOrder(UINT64_MAX);

    // Modify with invalid values
    assert(!engine.modifyOrder(113, -50, 50));
    assert(!engine.modifyOrder(113, 100, -10));
    assert(!engine.modifyOrder(99999, 100, 50));

    // Verify buy1 (113) remains completely valid and uncorrupted
    assert(engine.containsOrder(113));
    assert(engine.getOrderBook().size() == 1);

    engine.cancelOrder(113);
    assert(engine.getOrderBook().empty());

    std::cout << "TEST 34 PASSED: Invalid or boundary order inputs\n";
}

// ============================================================
// TEST 35: Stress-style sequence of operations
// ============================================================
{
    MatchingEngine engine;

    // Step 1: Add BUY 1 (118 @ 100 Qty 50) and BUY 2 (119 @ 99 Qty 50)
    Order buy1(118, 1, 1, Side::Buy, 100, 50);
    Order buy2(119, 2, 1, Side::Buy, 99, 50);
    engine.processOrder(buy1);
    engine.processOrder(buy2);

    assert(engine.containsOrder(118));
    assert(engine.containsOrder(119));
    assert(engine.getOrderBook().size() == 2);

    // Step 2: Process SELL 1 (120 @ 100 Qty 20) -> partial fill on buy1 (30 remaining)
    Order sell1(120, 3, 1, Side::Sell, 100, 20);
    engine.processOrder(sell1);

    assert(engine.containsOrder(118)); // buy1 remaining Qty 30
    assert(!engine.containsOrder(120));

    // Step 3: Modify buy1 (118) -> change price to 101, quantity to 40
    bool modified = engine.modifyOrder(118, 101, 40);
    assert(modified);
    assert(engine.containsOrder(118));

    // Step 4: Add BUY 3 (121 @ 101 Qty 30) behind buy1
    Order buy3(121, 4, 1, Side::Buy, 101, 30);
    engine.processOrder(buy3);
    assert(engine.containsOrder(121));

    // Step 5: Cancel buy2 (119 @ 99)
    engine.cancelOrder(119);
    assert(!engine.containsOrder(119));

    // Step 6: Process Market SELL (122 Qty 50) -> matches 40 against buy1 @ 101, and 10 against buy3 @ 101
    Order mktSell(122, 5, 1, Side::Sell, 0, 50, OrderType::Market);
    engine.processOrder(mktSell);

    // Step 7: Verify final state
    assert(!engine.containsOrder(118)); // buy1 fully filled
    assert(!engine.containsOrder(119)); // buy2 cancelled
    assert(engine.containsOrder(121));  // buy3 partially filled (20 remaining @ 101)
    assert(!engine.containsOrder(122)); // mktSell not resting

    assert(engine.getOrderBook().size() == 1); // Only price level 101 remains

    std::cout << "TEST 35 PASSED: Stress-style sequence of operations\n";
}

std::cout << "\nALL MATCHING ENGINE TESTS PASSED\n";

    return 0;
}