#pragma once

#include "OrderBook.hpp"

class MatchingEngine
{
private:

    OrderBook orderBook;

public:

    void processOrder(const Order& order);
    void processMarketOrder(const Order& order);
    void cancelOrder(const OrderID& orderID);
    bool modifyOrder(const OrderID& id, Price newPrice, Quantity newQuantity);
    bool containsOrder(const OrderID& orderID) const;

    const std::vector<Trade>& getTrades() const;
    void clearTrades();

    const OrderBook& getOrderBook() const;
    OrderBook& getOrderBook();
};