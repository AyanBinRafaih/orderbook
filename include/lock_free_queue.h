#pragma once
#include <atomic>
#include <vector>
#include <optional>
#include <cassert>

/*
single producer single consumer queue implementation
for push(), use one thread. for pop(), use a different thread
*/
template <typename T>
class SPSCQueue {
public:
    explicit SPSCQueue(size_t capacity)
        : capacity_(capacity + 1) // waste one slot due to full check
        , buffer_(capacity + 1)
        , head_(0)
        , tail_(0)
    {
        assert(capacity > 0);
    }


    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator = (const SPSCQueue&) = delete;

    /*
    pushing an item:
    - should return false if the queue is full
    - must be called from the producer thread
    */
    bool push(const T& item){
        const size_t tail = tail_.load(std::memory_order_relaxed);
        const size_t next_tail = advance(tail);

        if (next_tail == head_.load(std::memory_order_acquire)){
            return false; // queue is full
        }
        buffer_[tail] = item;
        tail_.store(next_tail, std::memory_order_release);
        return true; 
    }

    /*
    popping an item:
    - check if queue empty
    - must be called from the consumer thread
    */
    std::optional<T> pop() {
        const size_t head = head_.load(std::memory_order_relaxed);
        
        if (head == tail_.load(std::memory_order_acquire)){
            return std::nullopt;
        }
        T item = buffer_[head];
        head_.store(advance(head), std::memory_order_release);
        return item;
    }

    // size calculation is only an ESTIMATE since it may be stale by the time its is read by the caller
    size_t size_approx() const {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t head = head_.load(std::memory_order_relaxed);
        if (tail >= head){
            return tail - head;
        }
        return capacity_ - head + tail;
    }

    bool empty() const {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

    size_t capacity() const {
        return capacity_ - 1; // remember we did + 1 initially
    }
private:
    // index for the ring buffer
    size_t advance(size_t idx) const {
        return (idx + 1 == capacity_) ? 0 : idx + 1;
    }

    const size_t capacity_;
    std::vector<T> buffer_;

    // use separate cache lines for each idx
    // to avoid false sharing 
    // (one write by a given thread would invalidate the other thread's cache entry for that line provided they share a cache line)
    alignas(64) std::atomic<size_t> head_;
    alignas(64) std::atomic<size_t> tail_;
};