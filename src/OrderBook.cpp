#include "OrderBook.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <iostream>

OrderBook::OrderBook()
{
    trades.reserve(10000);
}

bool OrderBook::empty() const
{
    return buyBook.empty() && sellBook.empty();
}

std::size_t OrderBook::size() const
{
    return buyBook.size() + sellBook.size();
}

void OrderBook::print() const
{
    std::cout << "\n=== BUY BOOK ===\n";

    for(const auto& [price, level] : buyBook)
    {
        std::cout << "Price: " << price << "\n";

        for(const auto& order : level)
        {
            std::cout
                << "  ID: " << order.id
                << " Qty: " << order.remainingQuantity
                << "\n";
        }
    }

    std::cout << "\n=== SELL BOOK ====\n";

    for(const auto& [price, level] : sellBook)
    {
        std::cout << "Price: " << price << "\n";

        for(const auto& order : level)
        {
            std::cout
                << "  ID: " << order.id
                << " Qty: " << order.remainingQuantity
                << "\n";
        }
    }

    std::cout << "====\n";
}

void OrderBook::addOrder(const Order& order)
{
    if (order.remainingQuantity <= 0)
    {
        return;
    }

    if (order.type == OrderType::Limit && order.price <= 0)
    {
        return;
    }

    if (order.type == OrderType::Market)
    {
        processMarketOrder(order);
        return;
    }

    LOG_ORDER_ACCEPTED(order.id, order.side, order.price, order.remainingQuantity);

    Order incoming = order;

    // ==================== BUY ORDER MATCHING & RESTING LOGIC ====================
    if (incoming.side == Side::Buy)
    {
        auto bestAsk = sellBook.begin();

        while (incoming.remainingQuantity > 0 && 
               bestAsk != sellBook.end())
        {
            // Buy price cannot meet the lowest ask
            if (incoming.price < bestAsk->first)
                break;

            PriceLevel& level = bestAsk->second;

            Order& resting = level.front();

            Quantity traded =
                std::min(incoming.remainingQuantity,
                         resting.remainingQuantity);

            incoming.remainingQuantity -= traded;
            resting.remainingQuantity -= traded;

            trades.emplace_back(nextTradeId++, resting.id, incoming.id, resting.price, traded);
            LOG_TRADE_EXECUTED(trades.back());

            if (resting.remainingQuantity == 0)
            {
                orderMap.erase(resting.id);
                level.pop_front();
            }

            if (level.empty())
            {
                bestAsk = sellBook.erase(bestAsk);
            }
        }

        if (incoming.remainingQuantity > 0)
        {
            auto& level = buyBook[incoming.price];
            level.push_back(incoming);
            auto it = std::prev(level.end());
            orderMap[incoming.id] = {Side::Buy, incoming.price, it};
        }
    }
    // ==================== SELL ORDER MATCHING & RESTING LOGIC ====================
    else
    {
        auto bestBid = buyBook.begin();

        while (incoming.remainingQuantity > 0 &&
               bestBid != buyBook.end())
        {
            if (incoming.price > bestBid->first)
            {
                break;
            }
            PriceLevel& level = bestBid->second;

            Order& resting = level.front();

            Quantity traded =
                std::min(incoming.remainingQuantity,
                         resting.remainingQuantity);

            incoming.remainingQuantity -= traded;
            resting.remainingQuantity -= traded;

            trades.emplace_back(nextTradeId++, resting.id, incoming.id, resting.price, traded);
            LOG_TRADE_EXECUTED(trades.back());

            if (resting.remainingQuantity == 0)
            {
                orderMap.erase(resting.id);
                level.pop_front();
            }

            if (level.empty())
            {
                bestBid = buyBook.erase(bestBid);
            }
        }

        if (incoming.remainingQuantity > 0)
        {
            auto& level = sellBook[incoming.price];
            level.push_back(incoming);
            auto it = std::prev(level.end());
            orderMap[incoming.id] = {Side::Sell, incoming.price, it};
        }
    }
}

void OrderBook::processMarketOrder(const Order& order)
{
    if (order.remainingQuantity <= 0)
    {
        return;
    }

    Order incoming = order;

    // --- BUY MARKET ORDER MATCHING LOGIC ---
    if (incoming.side == Side::Buy)
    {
        auto bestAsk = sellBook.begin();

        while (incoming.remainingQuantity > 0 && 
               bestAsk != sellBook.end())
        {
            PriceLevel& level = bestAsk->second;

            Order& resting = level.front();

            Quantity traded =
                std::min(incoming.remainingQuantity,
                         resting.remainingQuantity);

            incoming.remainingQuantity -= traded;
            resting.remainingQuantity -= traded;

            trades.emplace_back(nextTradeId++, resting.id, incoming.id, resting.price, traded);
            LOG_TRADE_EXECUTED(trades.back());

            if (resting.remainingQuantity == 0)
            {
                orderMap.erase(resting.id);
                level.pop_front();
            }

            if (level.empty())
            {
                bestAsk = sellBook.erase(bestAsk);
            }
        }
    }
    // --- SELL MARKET ORDER MATCHING LOGIC ---
    else
    {
        auto bestBid = buyBook.begin();

        while (incoming.remainingQuantity > 0 &&
               bestBid != buyBook.end())
        {
            PriceLevel& level = bestBid->second;

            Order& resting = level.front();

            Quantity traded =
                std::min(incoming.remainingQuantity,
                         resting.remainingQuantity);

            incoming.remainingQuantity -= traded;
            resting.remainingQuantity -= traded;

            trades.emplace_back(nextTradeId++, resting.id, incoming.id, resting.price, traded);
            LOG_TRADE_EXECUTED(trades.back());

            if (resting.remainingQuantity == 0)
            {
                orderMap.erase(resting.id);
                level.pop_front();
            }

            if (level.empty())
            {
                bestBid = buyBook.erase(bestBid);
            }
        }
    }
}

void OrderBook::cancelOrder(const OrderID& id)
{
    auto it = orderMap.find(id);
    if (it == orderMap.end())
    {
        return;
    }

    LOG_ORDER_CANCELLED(id);

    const OrderLocation& loc = it->second;

    if (loc.side == Side::Buy)
    {
        auto bookIt = buyBook.find(loc.price);
        if (bookIt != buyBook.end())
        {
            auto& level = bookIt->second;
            level.erase(loc.iterator);
            if (level.empty())
            {
                buyBook.erase(bookIt);
            }
        }
    }
    else
    {
        auto bookIt = sellBook.find(loc.price);
        if (bookIt != sellBook.end())
        {
            auto& level = bookIt->second;
            level.erase(loc.iterator);
            if (level.empty())
            {
                sellBook.erase(bookIt);
            }
        }
    }

    orderMap.erase(it);
}

bool OrderBook::modifyOrder(const OrderID& id, Price newPrice, Quantity newQuantity)
{
    if (newPrice <= 0 || newQuantity <= 0)
    {
        return false;
    }

    auto it = orderMap.find(id);
    if (it == orderMap.end())
    {
        return false;
    }

    LOG_ORDER_MODIFIED(id, newPrice, newQuantity);

    OrderLocation& loc = it->second;

    if (newPrice == loc.price && newQuantity <= loc.iterator->remainingQuantity)
    {
        loc.iterator->remainingQuantity = newQuantity;
        loc.iterator->originalQuantity = newQuantity;
        return true;
    }

    Side side = loc.side;
    TraderID traderId = loc.iterator->traderId;
    SymbolID symbolId = loc.iterator->symbolId;

    Order updatedOrder(id, traderId, symbolId, side, newPrice, newQuantity);

    cancelOrder(id);
    addOrder(updatedOrder);

    return true;
}

bool OrderBook::containsOrder(const OrderID& orderID) const
{
    return orderMap.find(orderID) != orderMap.end();
}

const std::vector<Trade>& OrderBook::getTrades() const
{
    return trades;
}

void OrderBook::clearTrades()
{
    trades.clear();
}