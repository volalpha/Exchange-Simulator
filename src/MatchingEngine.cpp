#include "MatchingEngine.hpp"

const OrderBook& MatchingEngine::getOrderBook() const
{
    return orderBook;
}

void MatchingEngine::processOrder(const Order& order)
{
    orderBook.addOrder(order);
}

void MatchingEngine::processMarketOrder(const Order& order)
{
    orderBook.processMarketOrder(order);
}

void MatchingEngine::cancelOrder(const OrderID& orderID)
{
    orderBook.cancelOrder(orderID);
}

bool MatchingEngine::modifyOrder(const OrderID& id, Price newPrice, Quantity newQuantity)
{
    return orderBook.modifyOrder(id, newPrice, newQuantity);
}

bool MatchingEngine::containsOrder(const OrderID& orderID) const
{
    return orderBook.containsOrder(orderID);
}

const std::vector<Trade>& MatchingEngine::getTrades() const
{
    return orderBook.getTrades();
}

void MatchingEngine::clearTrades()
{
    orderBook.clearTrades();
}

OrderBook& MatchingEngine::getOrderBook()
{
    return orderBook;
}