#pragma once
#include "order.h"
#include "order_book.h"
#include "lock_free_queue.h"
#include "timer.h"
#include <thread>
#include <atomic>
#include <vector>

struct LatencySample {
    uint64_t cycles;
    char operation; // 'I' = insert, 'M' = match, 'C' = cancel
};

/*
consider a matching engine that's a single-threaded consumer
orders are submitted by one thread via submit()
these are read from the queue and then processed to see if there is a match
*/

class MatchingEngine {
public:
    explicit MatchingEngine(size_t queue_capacity = 1 << 20); // 1 million slots
    ~MatchingEngine();

    bool submit(const Order& order);
    
    void start();
    void stop();

    const std::vector<LatencySample>& latency_samples() const {
        return latency_log_;
    }

    const std::vector<Trade>& trades() const {
        return trades_;
    }

    void set_record_latency(bool enable) { // disable latency recording during warmup
        record_latency_ = enable;
    }

private:
    void run();

    SPSCQueue<Order> queue_;
    OrderBook book_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    std::vector<LatencySample> latency_log_;
    std::vector<Trade> trades_;
    bool record_latency_ = true;
    double cycles_per_ns = 1.0;
};