#pragma once
#include <map>
#include <list>
#include <unordered_map>
#include <vector>
#include <functional>

#include "order.hpp"
#include "Trade.hpp"

using PriceLevel = std::list<Order>;
using BuyBook = std::map<Price, PriceLevel, std::greater<Price>>;
using SellBook = std::map<Price, PriceLevel>;

struct OrderLocation
{
    Side side;
    Price price;
    std::list<Order>::iterator iterator;
};

class OrderBook
{
private:
    BuyBook buyBook;
    SellBook sellBook;
    std::unordered_map<OrderID, OrderLocation> orderMap;
    std::vector<Trade> trades;
    TradeID nextTradeId = 1;

public:
    void addOrder(const Order& order);
    void processMarketOrder(const Order& order);
    void cancelOrder(const OrderID& orderID);
    bool modifyOrder(const OrderID& id, Price newPrice, Quantity newQuantity);
    bool empty() const;
    std::size_t size() const;
    bool containsOrder(const OrderID& orderID) const;
    const std::vector<Trade>& getTrades() const;
    void clearTrades();
    void print() const;
};