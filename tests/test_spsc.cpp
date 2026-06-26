#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "lock_free_queue.h"


// check push and pop
TEST(SPSCQueue, PushPop) {
    SPSCQueue<int> q(16);
    EXPECT_TRUE(q.empty());
    EXPECT_TRUE(q.push(42));
    EXPECT_FALSE(q.empty());

    auto val = q.pop();
    EXPECT_TRUE(val.has_value());
    EXPECT_EQ(*val, 42);
    EXPECT_TRUE(q.empty());
}


// check if popping from an empty queue returns nullopt
TEST(SPSCQueue, PopEmpty) {
    SPSCQueue<int> q(16);
    auto val = q.pop();
    EXPECT_FALSE(val.has_value());
}


// check capacity when queue is full
TEST(SPSCQueue, FillToCapacity) {
    SPSCQueue<int> q(4);
    EXPECT_TRUE(q.push(1));
    EXPECT_TRUE(q.push(2));
    EXPECT_TRUE(q.push(3));
    EXPECT_TRUE(q.push(4));
    EXPECT_FALSE(q.push(5)); 
}


// order of elements should be FIFO
TEST(SPSCQueue, FIFOOrdering) {
    SPSCQueue<int> q(16);
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(q.push(i));
    }
    for (int i = 0; i < 10; ++i) {
        auto v = q.pop();
        ASSERT_TRUE(v.has_value());
        EXPECT_EQ(*v, i);
    }
}


// check for wrapping around the ring buffer
TEST(SPSCQueue, WrapAround) {
    SPSCQueue<int> q(4);

    for (int i = 0; i < 4; ++i) {
        q.push(i);
    }
    for (int i = 0; i < 4; ++i) {
        q.pop();
    }

    for (int i = 10; i < 14; ++i) {
        q.push(i);
    }
    for (int i = 10; i < 14; ++i) {
        auto v = q.pop();
        ASSERT_TRUE(v.has_value());
        EXPECT_EQ(*v, i);
    }
}


// calculate approximate size
TEST(SPSCQueue, ApproxSize) {
    SPSCQueue<int> q(16);
    EXPECT_EQ(q.size_approx(), 0u);
    q.push(1);
    q.push(2);
    q.push(3);
    EXPECT_EQ(q.size_approx(), 3u);
    q.pop();
    EXPECT_EQ(q.size_approx(), 2u);
}


// check if the multiple threads work correctly since producer and consumer run concurrently
// also check build is ok since ThreadSanatizer should catch data races if our memory ordering is wrong
TEST(SPSCQueue, ProducerConsumerCheck) {
    const int N = 1'000'000;
    SPSCQueue<int> q(65536);

    std::vector<int> received;
    received.reserve(N);

    std::thread producer([&]() {
        for (int i = 0; i < N; ++i) {
            while (!q.push(i)) {
                __asm__ volatile ("pause");
            }
        }
    });

    std::thread consumer([&]() {
        int count = 0;
        while (count < N) {
            auto val = q.pop();
            if (val) {
                received.push_back(*val);
                ++count;
            } else {
                __asm__ volatile ("pause");
            }
        }
    });

    producer.join();
    consumer.join();

    // check if all items arrived in order
    ASSERT_EQ(received.size(), static_cast<size_t>(N));
    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(received[i], i)
            << "Item at index " << i << " was " << received[i]
            << ", expected " << i;
    }
}