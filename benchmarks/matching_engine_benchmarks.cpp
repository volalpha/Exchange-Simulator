#include <algorithm>
#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

#include "MatchingEngine.hpp"

// Global volatile sink to prevent compiler dead-code elimination
volatile uint64_t g_benchmarkSink = 0;

// Helper to compute latency statistics in nanoseconds
struct LatencyStats {
  double minNs;
  double avgNs;
  double medianNs;
  double p90Ns;
  double p99Ns;
  double p999Ns;
  double maxNs;
};

LatencyStats computeLatencyStats(std::vector<double> &latenciesNs) {
  if (latenciesNs.empty())
    return {};

  std::sort(latenciesNs.begin(), latenciesNs.end());
  size_t count = latenciesNs.size();

  double sum = std::accumulate(latenciesNs.begin(), latenciesNs.end(), 0.0);
  double avg = sum / count;

  double minVal = latenciesNs.front();
  double maxVal = latenciesNs.back();
  double median = latenciesNs[static_cast<size_t>(count * 0.50)];
  double p90 = latenciesNs[static_cast<size_t>(count * 0.90)];
  double p99 = latenciesNs[static_cast<size_t>(count * 0.99)];
  double p999 = latenciesNs[static_cast<size_t>(
      std::min(count - 1, static_cast<size_t>(count * 0.999)))];

  return {minVal, avg, median, p90, p99, p999, maxVal};
}

void printBenchmarkHeader(const std::string &title) {
  std::cout << "\n==========================================================\n";
  std::cout << " BENCHMARK: " << title << "\n";
  std::cout << "==========================================================\n";
}

void printResults(size_t opsCount, double totalMs,
                  const LatencyStats *stats = nullptr) {
  double opsPerSec = (opsCount / (totalMs / 1000.0));
  double mopsPerSec = opsPerSec / 1'000'000.0;

  std::cout << std::fixed << std::setprecision(2);
  std::cout << "  Operations: " << opsCount << "\n";
  std::cout << "  Total Time: " << totalMs << " ms\n";
  std::cout << "  Throughput: " << opsPerSec << " ops/sec (" << mopsPerSec
            << " Mops/sec)\n";

  if (stats) {
    std::cout << "  Latency Profile:\n";
    std::cout << "    Min:    " << std::setprecision(1) << stats->minNs
              << " ns (" << (stats->minNs / 1000.0) << " us)\n";
    std::cout << "    Avg:    " << stats->avgNs << " ns ("
              << (stats->avgNs / 1000.0) << " us)\n";
    std::cout << "    p50:    " << stats->medianNs << " ns ("
              << (stats->medianNs / 1000.0) << " us)\n";
    std::cout << "    p90:    " << stats->p90Ns << " ns ("
              << (stats->p90Ns / 1000.0) << " us)\n";
    std::cout << "    p99:    " << stats->p99Ns << " ns ("
              << (stats->p99Ns / 1000.0) << " us)\n";
    std::cout << "    p99.9:  " << stats->p999Ns << " ns ("
              << (stats->p999Ns / 1000.0) << " us)\n";
    std::cout << "    Max:    " << stats->maxNs << " ns ("
              << (stats->maxNs / 1000.0) << " us)\n";
  }
}

// ------------------------------------------------------------
// BENCHMARK 1: Limit Order Insertion Throughput (Non-crossing)
// ------------------------------------------------------------
void benchLimitInsertion(size_t numOrders, size_t numPriceLevels) {
  printBenchmarkHeader("1. Limit Order Insertion (Non-Crossing)");

  // Pre-generate orders
  std::vector<Order> orders;
  orders.reserve(numOrders);

  for (size_t i = 0; i < numOrders; ++i) {
    OrderID id = i + 1;
    Price price = 10000 + (i % numPriceLevels); // non-crossing prices
    orders.emplace_back(id, 1, 1, Side::Buy, price, 100);
  }

  MatchingEngine engine;

  // Warmup
  for (size_t i = 0; i < 10000 && i < orders.size(); ++i) {
    engine.processOrder(orders[i]);
  }

  MatchingEngine benchEngine;
  std::vector<double> latenciesNs;
  latenciesNs.reserve(numOrders);

  auto start = std::chrono::high_resolution_clock::now();

  for (const auto &ord : orders) {
    auto t0 = std::chrono::high_resolution_clock::now();
    benchEngine.processOrder(ord);
    auto t1 = std::chrono::high_resolution_clock::now();

    latenciesNs.push_back(
        std::chrono::duration<double, std::nano>(t1 - t0).count());
  }

  auto end = std::chrono::high_resolution_clock::now();
  double totalMs =
      std::chrono::duration<double, std::milli>(end - start).count();

  g_benchmarkSink += benchEngine.getOrderBook().size();

  LatencyStats stats = computeLatencyStats(latenciesNs);
  printResults(numOrders, totalMs, &stats);
}

// ------------------------------------------------------------
// BENCHMARK 2: Matching Throughput (Full & Partial Fills)
// ------------------------------------------------------------
void benchMatchingThroughput(size_t numPairs) {
  printBenchmarkHeader("2. Matching Throughput (Continuous Matches)");

  // Pre-generate resting sell orders and aggressive buy orders
  std::vector<Order> sellOrders;
  std::vector<Order> buyOrders;
  sellOrders.reserve(numPairs);
  buyOrders.reserve(numPairs);

  for (size_t i = 0; i < numPairs; ++i) {
    OrderID sellId = i + 1;
    Price price = 1000 + (i % 100);
    sellOrders.emplace_back(sellId, 1, 1, Side::Sell, price, 50);

    OrderID buyId = numPairs + i + 1;
    buyOrders.emplace_back(buyId, 2, 1, Side::Buy, price, 50);
  }

  MatchingEngine engine;

  // Pre-populate resting orders (not timed)
  for (const auto &sell : sellOrders) {
    engine.processOrder(sell);
  }

  std::vector<double> latenciesNs;
  latenciesNs.reserve(numPairs);

  auto start = std::chrono::high_resolution_clock::now();

  for (const auto &buy : buyOrders) {
    auto t0 = std::chrono::high_resolution_clock::now();
    engine.processOrder(buy);
    auto t1 = std::chrono::high_resolution_clock::now();

    latenciesNs.push_back(
        std::chrono::duration<double, std::nano>(t1 - t0).count());
  }

  auto end = std::chrono::high_resolution_clock::now();
  double totalMs =
      std::chrono::duration<double, std::milli>(end - start).count();

  g_benchmarkSink += engine.getTrades().size();

  LatencyStats stats = computeLatencyStats(latenciesNs);
  printResults(numPairs, totalMs, &stats);
  std::cout << "  Trades Emitted: " << engine.getTrades().size() << "\n";
}

// ------------------------------------------------------------
// BENCHMARK 3: Cancellation Throughput (O(1) Order-ID Index)
// ------------------------------------------------------------
void benchCancellationThroughput(size_t numOrders) {
  printBenchmarkHeader("3. Order Cancellation Throughput");

  std::vector<Order> orders;
  std::vector<OrderID> cancelSequence;
  orders.reserve(numOrders);
  cancelSequence.reserve(numOrders);

  for (size_t i = 0; i < numOrders; ++i) {
    OrderID id = i + 1;
    Price price = 5000 + (i % 500);
    orders.emplace_back(id, 1, 1, Side::Buy, price, 100);
    cancelSequence.push_back(id);
  }

  // Shuffle cancellation sequence to simulate random cancellation
  std::mt19937 g(42);
  std::shuffle(cancelSequence.begin(), cancelSequence.end(), g);

  MatchingEngine engine;
  for (const auto &ord : orders) {
    engine.processOrder(ord);
  }

  std::vector<double> latenciesNs;
  latenciesNs.reserve(numOrders);

  auto start = std::chrono::high_resolution_clock::now();

  for (OrderID id : cancelSequence) {
    auto t0 = std::chrono::high_resolution_clock::now();
    engine.cancelOrder(id);
    auto t1 = std::chrono::high_resolution_clock::now();

    latenciesNs.push_back(
        std::chrono::duration<double, std::nano>(t1 - t0).count());
  }

  auto end = std::chrono::high_resolution_clock::now();
  double totalMs =
      std::chrono::duration<double, std::milli>(end - start).count();

  assert(engine.getOrderBook().empty());
  g_benchmarkSink += engine.getOrderBook().size();

  LatencyStats stats = computeLatencyStats(latenciesNs);
  printResults(numOrders, totalMs, &stats);
}

// ------------------------------------------------------------
// BENCHMARK 4: Mixed Real-World Workload Simulation
// ------------------------------------------------------------
void benchMixedWorkload(size_t numOps) {
  printBenchmarkHeader("4. Mixed Workload Simulation (60% Limit, 20% Cancel, "
                       "10% Market, 10% Modify)");

  MatchingEngine engine;
  std::mt19937 rng(1337);
  std::uniform_int_distribution<int> opDist(1, 100);
  std::uniform_int_distribution<Price> priceDist(90, 110);

  std::vector<OrderID> activeOrderIds;
  activeOrderIds.reserve(numOps);

  OrderID nextId = 1;
  size_t executedOps = 0;

  auto start = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < numOps; ++i) {
    int roll = opDist(rng);

    if (roll <= 60 || activeOrderIds.empty()) // 60%: Limit Order
    {
      Side side = (roll % 2 == 0) ? Side::Buy : Side::Sell;
      Price price = priceDist(rng);
      Order ord(nextId, 1, 1, side, price, 100);
      engine.processOrder(ord);

      if (engine.containsOrder(nextId)) {
        activeOrderIds.push_back(nextId);
      }
      nextId++;
    } else if (roll <= 80) // 20%: Cancel Order
    {
      size_t idx = rng() % activeOrderIds.size();
      OrderID idToCancel = activeOrderIds[idx];
      engine.cancelOrder(idToCancel);

      // Fast swap-and-pop removal from benchmark tracking array
      activeOrderIds[idx] = activeOrderIds.back();
      activeOrderIds.pop_back();
    } else if (roll <= 90) // 10%: Market Order
    {
      Side side = (roll % 2 == 0) ? Side::Buy : Side::Sell;
      Order mktOrd(nextId++, 2, 1, side, 0, 50, OrderType::Market);
      engine.processOrder(mktOrd);
    } else // 10%: Modify Order
    {
      size_t idx = rng() % activeOrderIds.size();
      OrderID idToMod = activeOrderIds[idx];
      Price newPrice = priceDist(rng);
      engine.modifyOrder(idToMod, newPrice, 80);
    }
    executedOps++;
  }

  auto end = std::chrono::high_resolution_clock::now();
  double totalMs =
      std::chrono::duration<double, std::milli>(end - start).count();

  g_benchmarkSink += engine.getTrades().size() + activeOrderIds.size();
  printResults(executedOps, totalMs, nullptr);
  std::cout << "  Trades Generated: " << engine.getTrades().size() << "\n";
  std::cout << "  Final Active Orders: " << engine.getOrderBook().size()
            << "\n";
}

// ------------------------------------------------------------
// BENCHMARK 5: Price Level Scalability Test
// ------------------------------------------------------------
void benchPriceLevelScalability() {
  printBenchmarkHeader("5. Price Level Scalability Test (10 to 10,000 Levels)");

  std::vector<size_t> priceLevelCounts = {10, 100, 1000, 10000};
  const size_t ordersPerTest = 100000;

  for (size_t levels : priceLevelCounts) {
    std::vector<Order> orders;
    orders.reserve(ordersPerTest);

    for (size_t i = 0; i < ordersPerTest; ++i) {
      OrderID id = i + 1;
      Price price = 10000 + (i % levels);
      orders.emplace_back(id, 1, 1, Side::Buy, price, 50);
    }

    MatchingEngine engine;
    auto start = std::chrono::high_resolution_clock::now();

    for (const auto &ord : orders) {
      engine.processOrder(ord);
    }

    auto end = std::chrono::high_resolution_clock::now();
    double totalMs =
        std::chrono::duration<double, std::milli>(end - start).count();
    double mops = (ordersPerTest / (totalMs / 1000.0)) / 1'000'000.0;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Price Levels: " << std::setw(6) << levels
              << " | Total Time: " << std::setw(7) << totalMs << " ms"
              << " | Throughput: " << std::setw(6) << mops << " Mops/sec\n";
  }
}

int main() {
  std::cout << "==========================================================\n";
  std::cout << "      EXCHANGE SIMULATOR PERFORMANCE BENCHMARK SUITE       \n";
  std::cout << "==========================================================\n";

  // Run benchmarks
  benchLimitInsertion(100000, 1000);
  benchMatchingThroughput(50000);
  benchCancellationThroughput(100000);
  benchMixedWorkload(200000);
  benchPriceLevelScalability();

  std::cout << "\n==========================================================\n";
  std::cout << " ALL BENCHMARKS COMPLETED SUCCESSFULLY\n";
  std::cout << " (Sink accumulator: " << g_benchmarkSink << ")\n";
  std::cout << "==========================================================\n";

  return 0;
}
