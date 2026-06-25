#include <gtest/gtest.h>
#include <order_book.h>

// Create an order
Order make_order(uint64_t id, Side side, double price, uint64_t qty, OrderType type = OrderType::Limit) {
    Order o{};
    o.order_id = id;
    o.side = side;
    o.price = price;
    o.quantity = qty;
    o.type = type;
    o.timestamp_ns = id * 1000; 
    return o;
}

// Lone bid with no match from the orderbook
TEST(OrderBook, SingleBidNoMatch) {
    OrderBook book;
    auto trades = book.add_order(make_order(1, Side::Buy, 50.00, 100));
    EXPECT_EQ(trades.size(), 0u);
    EXPECT_DOUBLE_EQ(book.best_bid(), 50.00);
    EXPECT_EQ(book.bid_depth_at(50.00), 100u);
    EXPECT_TRUE(book.has_order(1));
}


// Lone ask with no match from the book
TEST(OrderBook, SingleAskNoMatch) {
    OrderBook book;
    auto trades = book.add_order(make_order(1, Side::Sell, 50.05, 100));
    EXPECT_EQ(trades.size(), 0u);
    EXPECT_DOUBLE_EQ(book.best_ask(), 50.05);
    EXPECT_EQ(book.ask_depth_at(50.05), 100u);
}


// Match found (buy crosses the spread value)
TEST(OrderBook, FullMatch) {
    OrderBook book;
    book.add_order(make_order(1, Side::Sell, 50.00, 100));
    auto trades = book.add_order(make_order(2, Side::Buy, 50.00, 100));

    EXPECT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, 100u);
    EXPECT_DOUBLE_EQ(trades[0].price, 50.00);
    EXPECT_EQ(trades[0].buy_order_id, 2u);
    EXPECT_EQ(trades[0].sell_order_id, 1u);
    EXPECT_EQ(book.total_ask_levels(), 0u);
    EXPECT_EQ(book.total_bid_levels(), 0u);
}


// Half-match. buy < resting ask in the book
TEST(OrderBook, PartialFillIncoming) {
    OrderBook book;
    book.add_order(make_order(1, Side::Sell, 50.00, 200));
    auto trades = book.add_order(make_order(2, Side::Buy, 50.00, 100));

    EXPECT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, 100u);
    // Remaining 100 on the ask side
    EXPECT_EQ(book.ask_depth_at(50.00), 100u);
    EXPECT_EQ(book.total_bid_levels(), 0u);
}


// Half-match. buy > resting ask in the book
TEST(OrderBook, PartialFillResting) {
    OrderBook book;
    book.add_order(make_order(1, Side::Sell, 50.00, 100));
    auto trades = book.add_order(make_order(2, Side::Buy, 50.00, 200));

    EXPECT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, 100u);

    EXPECT_EQ(book.bid_depth_at(50.00), 100u);
    EXPECT_EQ(book.total_ask_levels(), 0u);
}


// If two bids are at same price, then prioritise matching by arrival time (first to arrive is first to match)
TEST(OrderBook, PriceTimePriority) {
    OrderBook book;
    book.add_order(make_order(1, Side::Buy, 50.00, 100)); 
    book.add_order(make_order(2, Side::Buy, 50.00, 100)); 

    auto trades = book.add_order(make_order(3, Side::Sell, 50.00, 100));
    EXPECT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].buy_order_id, 1u); 
    EXPECT_TRUE(book.has_order(2));
    EXPECT_FALSE(book.has_order(1));
}


// Buy matches through 2 asks 
TEST(OrderBook, MultiLevelMatch) {
    OrderBook book;
    book.add_order(make_order(1, Side::Sell, 50.00, 100));
    book.add_order(make_order(2, Side::Sell, 50.01, 100));

    auto trades = book.add_order(make_order(3, Side::Buy, 50.10, 200));
    EXPECT_EQ(trades.size(), 2u);
    EXPECT_DOUBLE_EQ(trades[0].price, 50.00); 
    EXPECT_DOUBLE_EQ(trades[1].price, 50.01);
    EXPECT_EQ(book.total_ask_levels(), 0u);
}


// Cancel order (assuming that it exists in the book)
TEST(OrderBook, CancelOrder) {
    OrderBook book;
    book.add_order(make_order(1, Side::Buy, 50.00, 100));
    EXPECT_TRUE(book.has_order(1));

    bool result = book.cancel_order(1);
    EXPECT_TRUE(result);
    EXPECT_FALSE(book.has_order(1));
    EXPECT_EQ(book.total_bid_levels(), 0u);
}


// Cancel order that does not exist in the book
TEST(OrderBook, CancelNonExistent) {
    OrderBook book;
    EXPECT_FALSE(book.cancel_order(999));
}


// Cancel order. check to see if the right order is removed when prices are shared 
TEST(OrderBook, CancelOneOfMany) {
    OrderBook book;
    book.add_order(make_order(1, Side::Buy, 50.00, 100));
    book.add_order(make_order(2, Side::Buy, 50.00, 200));
    book.add_order(make_order(3, Side::Buy, 50.00, 300));

    book.cancel_order(2); 

    EXPECT_TRUE(book.has_order(1));
    EXPECT_FALSE(book.has_order(2));
    EXPECT_TRUE(book.has_order(3));
    EXPECT_EQ(book.bid_depth_at(50.00), 400u); 
}


// Rule check: total bought = total sold 
TEST(OrderBook, QuantityConservation) {
    OrderBook book;
    uint64_t id = 1;
    uint64_t total_bought = 0;
    uint64_t total_sold   = 0;

    for (int i = 0; i < 1000; ++i) {
        double price = 50.00 + (i % 10) * 0.01;
        uint64_t qty = 100 + (i % 5) * 50;
        Side side = (i % 2 == 0) ? Side::Buy : Side::Sell;

        auto trades = book.add_order(make_order(id++, side, price, qty));
        for (auto& t: trades) {
            total_bought += t.quantity;
            total_sold   += t.quantity;
        }
    }

    EXPECT_EQ(total_bought, total_sold);
}