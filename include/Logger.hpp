#pragma once

#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "types.hpp"
#include "Trade.hpp"

enum class LogEventType
{
    OrderAccepted,
    OrderCancelled,
    OrderModified,
    TradeExecuted
};

class Logger
{
private:
    std::ofstream logFile;
    bool enabled{false};

    Logger() = default;

    std::string getTimestamp() const
    {
        auto now = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << std::put_time(std::gmtime(&timeT), "%Y-%m-%dT%H:%M:%S")
            << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
        return oss.str();
    }

public:
    ~Logger()
    {
        close();
    }

    static Logger& getInstance()
    {
        static Logger instance;
        return instance;
    }

    bool open(const std::string& filename)
    {
        close();
        logFile.open(filename, std::ios::out | std::ios::app);
        enabled = logFile.is_open();
        return enabled;
    }

    void close()
    {
        if (logFile.is_open())
        {
            logFile.flush();
            logFile.close();
        }
        enabled = false;
    }

    bool isEnabled() const
    {
        return enabled;
    }

    void logOrderAccepted(const OrderID& id, Side side, Price price, Quantity qty)
    {
        if (!enabled) return;
        logFile << getTimestamp() << " [ORDER_ACCEPTED] OrderID=" << id
                << " Side=" << (side == Side::Buy ? "BUY" : "SELL")
                << " Price=" << price
                << " Qty=" << qty << "\n";
    }

    void logOrderCancelled(const OrderID& id)
    {
        if (!enabled) return;
        logFile << getTimestamp() << " [ORDER_CANCELLED] OrderID=" << id << "\n";
    }

    void logOrderModified(const OrderID& id, Price newPrice, Quantity newQty)
    {
        if (!enabled) return;
        logFile << getTimestamp() << " [ORDER_MODIFIED] OrderID=" << id
                << " NewPrice=" << newPrice
                << " NewQty=" << newQty << "\n";
    }

    void logTradeExecuted(const Trade& trade)
    {
        if (!enabled) return;
        logFile << getTimestamp() << " [TRADE_EXECUTED] TradeID=" << trade.tradeId
                << " MakerOrderID=" << trade.makerOrderId
                << " TakerOrderID=" << trade.takerOrderId
                << " Price=" << trade.price
                << " Qty=" << trade.quantity << "\n";
    }
};

#ifdef ENABLE_LOGGING
    #define LOG_ORDER_ACCEPTED(id, side, price, qty) Logger::getInstance().logOrderAccepted(id, side, price, qty)
    #define LOG_ORDER_CANCELLED(id) Logger::getInstance().logOrderCancelled(id)
    #define LOG_ORDER_MODIFIED(id, newPrice, newQty) Logger::getInstance().logOrderModified(id, newPrice, newQty)
    #define LOG_TRADE_EXECUTED(trade) Logger::getInstance().logTradeExecuted(trade)
#else
    #define LOG_ORDER_ACCEPTED(id, side, price, qty) ((void)0)
    #define LOG_ORDER_CANCELLED(id) ((void)0)
    #define LOG_ORDER_MODIFIED(id, newPrice, newQty) ((void)0)
    #define LOG_TRADE_EXECUTED(trade) ((void)0)
#endif
