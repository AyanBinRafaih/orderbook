#pragma once
#include "order.h"
#include <map>
#include <list>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <functional>

// For all orders at a given price (sorted by arrival time) --> oldest first
struct PriceLevel {
    double price;
    std::list<Order> orders;
    uint64_t total_quantity = 0;
};

// Orderbook and operations
class OrderBook {
public:
    OrderBook() = default;
    std::vector<Trade> add_order(const Order& order); // Insert a new order. Returns all completed trades
    bool cancel_order(uint64_t order_id); // Cancel an order

    // Read-only
    double best_bid() const;
    double best_ask() const;
    uint64_t bid_depth_at(double price) const;
    uint64_t ask_depth_at(double price) const;
    bool has_order(uint64_t order_id) const;
    size_t total_bid_levels() const;
    size_t total_ask_levels() const;

private:
    // Bids. highest price first
    std::map<double, PriceLevel, std::greater<double>> bids_;
    std::map<double, PriceLevel> asks_; // Asks. lowest price first
    std::unordered_map<uint64_t, std::pair<bool, std::list<Order>::iterator>> order_map_; // order_id. true if a bid, false if a asl
    
    mutable std::mutex mutex_;
    std::vector<Trade> match(Order& incoming); // match a new order against the book
};