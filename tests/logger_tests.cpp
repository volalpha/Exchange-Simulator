#include <cassert>
#include <iostream>
#include <fstream>
#include <string>

#include "MatchingEngine.hpp"
#include "Logger.hpp"

int main()
{
    const std::string logFileName = "test_execution.log";

    // Clean up old log file if present
    std::remove(logFileName.c_str());

    bool opened = Logger::getInstance().open(logFileName);
    assert(opened);
    assert(Logger::getInstance().isEnabled());

    MatchingEngine engine;

    Order buy1(1, 101, 1, Side::Buy, 100, 50);
    engine.processOrder(buy1);
    assert(engine.containsOrder(1));

    bool modified = engine.modifyOrder(1, 101, 40);
    assert(modified);

    Order sell1(2, 102, 1, Side::Sell, 100, 20);
    engine.processOrder(sell1);
    assert(engine.getTrades().size() == 1);

    engine.cancelOrder(1);
    assert(!engine.containsOrder(1));

    Logger::getInstance().close();

    std::ifstream inFile(logFileName);
    assert(inFile.is_open());

    std::string line;
    bool foundAccepted = false;
    bool foundModified = false;
    bool foundTrade = false;
    bool foundCancelled = false;

    while (std::getline(inFile, line))
    {
        if (line.find("[ORDER_ACCEPTED]") != std::string::npos) foundAccepted = true;
        if (line.find("[ORDER_MODIFIED]") != std::string::npos) foundModified = true;
        if (line.find("[TRADE_EXECUTED]") != std::string::npos) foundTrade = true;
        if (line.find("[ORDER_CANCELLED]") != std::string::npos) foundCancelled = true;
    }

    inFile.close();

    assert(foundAccepted);
    assert(foundModified);
    assert(foundTrade);
    assert(foundCancelled);

    std::cout << "LOGGER TESTS PASSED: All lifecycle events logged to " << logFileName << "\n";

    return 0;
}
