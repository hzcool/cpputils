//
// Created by hzcool on 22-4-28.
//

#ifndef CPPUTILS_SAFE_QUEUE_H
#define CPPUTILS_SAFE_QUEUE_H

#include <queue>
#include <vector>
#include <mutex>
#include <atomic>

template<typename Tp, typename Container = std::queue<Tp>>
class SafeQueue {
public:

    void put(const Tp& x) {
        std::lock_guard<std::mutex> lock(mtx);
        c.push(x);
    }

    void put(Tp&& x) {
        std::lock_guard<std::mutex> lock(mtx);
        c.push(std::move(x));
    }

    bool get(Tp& x) {
        std::lock_guard<std::mutex> lock(mtx);
        if(c.empty()) return false;
        x = c.front();
        c.pop();
        return true;
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(mtx);
        return c.size();
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mtx);
        return c.empty();
    }
private:
    Container c;
    std::mutex mtx;
};


template<typename Tp, typename Comp = std::less<Tp>>
class PriorityQueueAdapter: public std::priority_queue<Tp, std::vector<Tp>, Comp>{
public:
    using Base = std::priority_queue<Tp, std::vector<Tp>, Comp>;
    using const_reference = typename Base::const_reference;
    const_reference front() const {
        return this->top();
    }
};

template<typename Tp, typename Comp = std::less<Tp>>
class SafePriorityQueue: public SafeQueue<Tp, PriorityQueueAdapter<Tp, Comp>> {};


#include <atomic>

template <typename T,unsigned CAPACITY>
class SignlePVQueue {
public:
    SignlePVQueue(): sz(0), head(0), tail(0) {}

    bool put(const T& x) {
        if(sz.load(std::memory_order_relaxed) >= CAPACITY) return false;
        data[tail] = x;
        if((++tail) == CAPACITY) tail = 0;
        sz.fetch_add(1, std::memory_order_release);
        return true;
    }

    bool put(T&& x) {
        if(sz.load(std::memory_order_relaxed) >= CAPACITY) return false;
        data[tail] = std::move(x);
        if((++tail) == CAPACITY) tail = 0;
        sz.fetch_add(1, std::memory_order_release);
        return true;
    }

    bool get(T& x) {
        if(sz.load(std::memory_order_relaxed) <= 0) return false;
        x = std::move(data[head]);
        if((++head) == CAPACITY) head = 0;
        sz.fetch_sub(1, std::memory_order_release);
        return true;
    }

    bool empty()   {
        return sz.load(std::memory_order_relaxed) == 0;
    }

    size_t size() {
        return sz.load(std::memory_order_relaxed);
    }

private:
    std::atomic<unsigned> sz;
    unsigned head, tail;
    T data[CAPACITY];
};


#endif //CPPUTILS_SAFE_QUEUE_H
