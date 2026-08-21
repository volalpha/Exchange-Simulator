#include <cassert>
#include <iostream>
#include "OrderBook.hpp"

int main()
{
    // =========================================
    // TEST 1: BUY ORDER RESTS
    // =========================================

    {
        OrderBook book;

        Order buy(
            1,
            101,
            1,
            Side::Buy,
            100,
            50
        );

        book.addOrder(buy);

        assert(book.size() == 1);
        std::cout << "TEST 1 PASSED: Buy rests\n";
    }


    // =========================================
    // TEST 2: SELL ORDER RESTS
    // =========================================

    {
        OrderBook book;

        Order sell(
            2,
            102,
            1,
            Side::Sell,
            105,
            40
        );

        book.addOrder(sell);

        assert(book.size() == 1);
        std::cout << "TEST 2 PASSED: Sell rests\n";
    }


    // =========================================
    // TEST 3: FULL MATCH
    // =========================================

    {
        OrderBook book;

        Order buy(
            1,
            101,
            1,
            Side::Buy,
            100,
            50
        );

        Order sell(
            2,
            102,
            1,
            Side::Sell,
            100,
            50
        );

        book.addOrder(buy);
        book.addOrder(sell);

        assert(book.empty());

        std::cout << "TEST 3 PASSED: Full match\n";
    }


    // =========================================
    // TEST 4: PARTIAL MATCH
    // =========================================

    {
        OrderBook book;

        Order buy(
            1,
            101,
            1,
            Side::Buy,
            100,
            100
        );

        Order sell(
            2,
            102,
            1,
            Side::Sell,
            100,
            40
        );

        book.addOrder(buy);
        book.addOrder(sell);

        // Remaining BUY order should rest
        assert(book.size() == 1);

        std::cout << "TEST 4 PASSED: Partial match\n";

        book.print();
    }


    // =========================================
    // TEST 5: CANCEL BUY
    // =========================================

    {
        OrderBook book;

        Order buy(
            1,
            101,
            1,
            Side::Buy,
            100,
            50
        );

        book.addOrder(buy);

        assert(book.size() == 1);

        book.cancelOrder(1);

        assert(book.empty());

        std::cout << "TEST 5 PASSED: Cancel buy\n";
    }


    // =========================================
    // TEST 6: CANCEL SELL
    // =========================================

    {
        OrderBook book;

        Order sell(
            2,
            102,
            1,
            Side::Sell,
            105,
            40
        );

        book.addOrder(sell);

        assert(book.size() == 1);

        book.cancelOrder(2);

        assert(book.empty());

        std::cout << "TEST 6 PASSED: Cancel sell\n";
    }


    // =========================================
    // TEST 7: CANCEL NON-EXISTENT ORDER
    // =========================================

    {
        OrderBook book;

        Order buy(
            1,
            101,
            1,
            Side::Buy,
            100,
            50
        );

        book.addOrder(buy);

        book.cancelOrder(999);

        assert(book.size() == 1);

        std::cout << "TEST 7 PASSED: Cancel non-existent order\n";
    }

    // =========================================
// TEST 8: FIFO AT SAME PRICE
// =========================================

{
    OrderBook book;

    Order buy1(
        1,
        101,
        1,
        Side::Buy,
        100,
        50
    );

    Order buy2(
        2,
        102,
        1,
        Side::Buy,
        100,
        50
    );

    Order sell(
        3,
        103,
        1,
        Side::Sell,
        100,
        60
    );

    book.addOrder(buy1);
    book.addOrder(buy2);

    book.addOrder(sell);

    assert(book.size() == 1);

    std::cout << "TEST 8 PASSED: FIFO at same price\n";

    book.print();
}

// TEST 9: PRICE PRIORITY
{
    OrderBook book;

    Order sell1(1, 101, 1, Side::Sell, 105, 50);
    Order sell2(2, 102, 1, Side::Sell, 102, 50);

    book.addOrder(sell1);
    book.addOrder(sell2);

    Order buy(3, 103, 1, Side::Buy, 103, 30);
    book.addOrder(buy);

    book.print();

    assert(book.size() == 2);

    std::cout << "TEST 9 PASSED: Price priority\n";
}
// TEST 10: MULTIPLE PRICE LEVELS
{
    OrderBook book;

    Order sell1(1, 101, 1, Side::Sell, 101, 30);
    Order sell2(2, 102, 1, Side::Sell, 102, 40);

    book.addOrder(sell1);
    book.addOrder(sell2);

    Order buy(3, 103, 1, Side::Buy, 105, 60);
    book.addOrder(buy);

    book.print();

    assert(book.size() == 1);

    std::cout << "TEST 10 PASSED: Multiple price levels\n";
}
// TEST 11: PARTIAL FILL ACROSS MULTIPLE LEVELS
{
    OrderBook book;

    book.addOrder(Order(1, 101, 1, Side::Sell, 101, 30));
    book.addOrder(Order(2, 102, 1, Side::Sell, 102, 40));

    book.addOrder(Order(3, 103, 1, Side::Buy, 105, 50));

    book.print();

    assert(book.size() == 1);

    std::cout << "TEST 11 PASSED: Partial fill across levels\n";
}
// TEST 12: FULL FILL ACROSS MULTIPLE LEVELS
{
    OrderBook book;

    book.addOrder(Order(1, 101, 1, Side::Sell, 101, 20));
    book.addOrder(Order(2, 102, 1, Side::Sell, 102, 30));

    book.addOrder(Order(3, 103, 1, Side::Buy, 105, 50));

    book.print();

    assert(book.empty());

    std::cout << "TEST 12 PASSED: Full fill across levels\n";
}
// TEST 13: SELL PRICE PRIORITY
{
    OrderBook book;

    book.addOrder(Order(1, 101, 1, Side::Buy, 105, 30));
    book.addOrder(Order(2, 102, 1, Side::Buy, 102, 40));

    book.addOrder(Order(3, 103, 1, Side::Sell, 100, 50));

    book.print();

    assert(book.size() == 1);

    std::cout << "TEST 13 PASSED: Sell price priority\n";
}
// TEST 14: PRICE PRIORITY + FIFO
{
    OrderBook book;

    // Same price: ID 1 must execute before ID 2
    book.addOrder(Order(1, 101, 1, Side::Buy, 100, 40));
    book.addOrder(Order(2, 102, 1, Side::Buy, 100, 30));

    // Worse price
    book.addOrder(Order(3, 103, 1, Side::Buy, 99, 50));

    book.addOrder(Order(4, 104, 1, Side::Sell, 100, 60));

    book.print();

    assert(book.size() == 2);

    std::cout << "TEST 14 PASSED: Price priority + FIFO\n";
}
// TEST 15: NON-CROSSING ORDER RESTS
{
    OrderBook book;

    book.addOrder(Order(1, 101, 1, Side::Sell, 105, 50));
    book.addOrder(Order(2, 102, 1, Side::Buy, 100, 30));

    book.print();

    assert(book.size() == 2);

    std::cout << "TEST 15 PASSED: Non-crossing order rests\n";
}
    std::cout << "\nALL TESTS PASSED\n";

    return 0;
}