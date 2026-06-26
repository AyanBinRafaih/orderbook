#include "order_book.h"
#include <cassert>

std::vector<Trade> OrderBook::add_order(const Order& order) {
    std::lock_guard<std::mutex> lock(mutex_);

    Order mutable_order = order;
    std::vector<Trade> trades = match(mutable_order);

    // Inserting remaining order parts if not fully filled with initial match 
    if (mutable_order.quantity > 0 && mutable_order.type == OrderType::Limit) {
        if (mutable_order.side == Side::Buy) {
            auto& level = bids_[mutable_order.price];
            level.price = mutable_order.price;
            level.total_quantity += mutable_order.quantity;

            level.orders.push_back(mutable_order);
            auto it = std::prev(level.orders.end());

            order_map_[mutable_order.order_id] = {true, it};
        } 
        else {
            auto& level = asks_[mutable_order.price];
            level.price = mutable_order.price;
            level.total_quantity += mutable_order.quantity;

            level.orders.push_back(mutable_order);
            auto it = std::prev(level.orders.end());

            order_map_[mutable_order.order_id] = {false, it};
        }
    }
    return trades;
}

std::vector<Trade> OrderBook::match(Order& incoming) {
    std::vector<Trade> trades;

    if (incoming.side == Side::Buy) {
        // Try to match against the asks (lowest ask first)
        while (incoming.quantity > 0 && !asks_.empty()) {
            auto& [ask_price, ask_level] = *asks_.begin();

            if (incoming.price < ask_price && incoming.type != OrderType::Market) {
                break;
            }
            // Match against orders at the given price (with respect to time priority)
            while ((incoming.quantity > 0) && !ask_level.orders.empty()) {
                Order& resting = ask_level.orders.front();
                uint64_t fill_qty = std::min(incoming.quantity, resting.quantity);

                Trade trade;
                trade.buy_order_id  = incoming.order_id;
                trade.sell_order_id = resting.order_id;
                trade.price         = ask_price;
                trade.quantity      = fill_qty;
                trade.timestamp_ns  = incoming.timestamp_ns;
                trades.push_back(trade);

                incoming.quantity       -= fill_qty;
                resting.quantity        -= fill_qty;
                ask_level.total_quantity -= fill_qty;

                if (resting.quantity == 0) {
                    order_map_.erase(resting.order_id);
                    ask_level.orders.pop_front();
                }
            }

            if (ask_level.orders.empty()) {
                asks_.erase(asks_.begin());
            }
        }
    } 
    else {
        // Try to match against the bids (highest bid first)
        while (incoming.quantity > 0 && !bids_.empty()) {
            auto& [bid_price, bid_level] = *bids_.begin();

            if (incoming.price > bid_price && incoming.type != OrderType::Market) {
                break;
            }

            while (incoming.quantity > 0 && !bid_level.orders.empty()) {
                Order& resting = bid_level.orders.front();
                uint64_t fill_qty = std::min(incoming.quantity, resting.quantity);

                Trade trade;
                trade.buy_order_id  = resting.order_id;
                trade.sell_order_id = incoming.order_id;
                trade.price         = bid_price;
                trade.quantity      = fill_qty;
                trade.timestamp_ns  = incoming.timestamp_ns;
                trades.push_back(trade);

                incoming.quantity       -= fill_qty;
                resting.quantity        -= fill_qty;
                bid_level.total_quantity -= fill_qty;

                if (resting.quantity == 0) {
                    order_map_.erase(resting.order_id);
                    bid_level.orders.pop_front();
                }
            }

            if (bid_level.orders.empty()) {
                bids_.erase(bids_.begin());
            }
        }
    }

    return trades;
}

bool OrderBook::cancel_order(uint64_t order_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = order_map_.find(order_id);
    if (it == order_map_.end()) return false;

    auto [in_bids, order_it] = it->second;
    double price = order_it->price;
    uint64_t qty = order_it->quantity;

    if (in_bids) {
        auto level_it = bids_.find(price);
        level_it->second.total_quantity -= qty;
        level_it->second.orders.erase(order_it);

        if (level_it->second.orders.empty()) {
            bids_.erase(level_it);
        }
    } 
    else {
        auto level_it = asks_.find(price);
        level_it->second.total_quantity -= qty;
        level_it->second.orders.erase(order_it);

        if (level_it->second.orders.empty()) {
            asks_.erase(level_it);
        }
    }

    order_map_.erase(it);
    return true;
}

double OrderBook::best_bid() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (bids_.empty()) {
        return 0.0;
    }
    return bids_.begin()->first;
}

double OrderBook::best_ask() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (asks_.empty()) {
        return 0.0;
    }
    return asks_.begin()->first;
}

uint64_t OrderBook::bid_depth_at(double price) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = bids_.find(price);
    if (it == bids_.end()){
        return 0;
    }
    return it->second.total_quantity;
}

uint64_t OrderBook::ask_depth_at(double price) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = asks_.find(price);
    if (it == asks_.end()){
        return 0;
    }
    return it->second.total_quantity;
}

bool OrderBook::has_order(uint64_t order_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return order_map_.count(order_id) > 0;
}

size_t OrderBook::total_bid_levels() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return bids_.size();
}

size_t OrderBook::total_ask_levels() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return asks_.size();
}