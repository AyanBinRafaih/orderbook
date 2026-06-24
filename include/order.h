#pragma once
#include <cstdint>

// Market side (either Buy or Sell)
enum class Side: uint8_t {
    Buy = 0,
    Sell = 1
};

// Order type
enum class OrderType: uint8_t {
    Limit = 0,
    Market = 1,
    Cancel = 2
};

// Order submitted to the book
// Force the struct to be exactly one cache line (64 bytes)
// So that one order is never caught between 2 cache lines
struct alignas(64) Order {
    uint64_t order_id; 
    double price;
    uint64_t quantity;
    uint64_t timestamp_ns;
    Side side;
    OrderType type;
};

static_assert(sizeof(Order) == 64, "Order must fit in one cache line");

// A completed trade when a match is found between two orders
struct Trade {
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    double price;
    uint64_t quantity;
    uint64_t timestamp_ns;
};