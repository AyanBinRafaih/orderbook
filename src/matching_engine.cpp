#include "matching_engine.h"

MatchingEngine::MatchingEngine(size_t queue_capacity)
    : queue_(queue_capacity)

{
    latency_log_.reserve(10'000'000);
    trades_.reserve(1'000'000);
    cycles_per_ns = timer::calibrate_cycles_per_ns();
}

MatchingEngine::~MatchingEngine() {
    if (running_){
        stop();
    }
}

bool MatchingEngine::submit(const Order& order){
    return queue_.push(order);
}

void MatchingEngine::start() {
    running_.store(true, std::memory_order_release);
    thread_ = std::thread(&MatchingEngine::run, this);
}

void MatchingEngine::stop() {
    running_.store(false, std::memory_order_release);
    if (thread_.joinable()){
        thread_.join();
    }
}

void MatchingEngine::run() {
    // should run until running_ = false AND queue is empty
    while (running_.load(std::memory_order_acquire) || !queue_.empty()){
        auto maybe_order = queue_.pop();
        if (!maybe_order){
            // queue is empty so spin with a pause-CPU instruction
            __asm__ volatile ("pause");
            continue;
        }

        const Order& order = *maybe_order;

        if (order.type == OrderType::Cancel){
            uint64_t t0 = timer::start();
            book_.cancel_order(order.order_id);
            uint64_t t1 = timer::stop();

            if (record_latency_){
                latency_log_.push_back({t1 - t0, 'C'});
            }
        }
        else {
            uint64_t t0 = timer::start();
            auto new_trades = book_.add_order(order);
            uint64_t t1 = timer::stop();

            bool matched_status = !new_trades.empty();
            if (record_latency_){
                latency_log_.push_back({t1 - t0, matched_status ? 'M' : 'I'});
            }

            for (auto& all_trades: new_trades){
                trades_.push_back(all_trades);
            }
        }
    }
}