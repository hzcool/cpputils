//
// Created by hzcool on 22-4-28.
//

#ifndef CPPUTILS_SAFE_QUEUE_H
#define CPPUTILS_SAFE_QUEUE_H

#include <queue>
#include <vector>
#include <mutex>
#include <atomic>
#include <condition_variable>

template<typename Tp, typename Container = std::queue<Tp>>
class SafeQueue {
public:

    bool put(const Tp& x) {
        std::lock_guard<std::mutex> lock(mtx);
        c.push(x);
        return true;
    }

    bool put(Tp&& x) {
        std::lock_guard<std::mutex> lock(mtx);
        c.push(std::move(x));
        return true;
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



template<typename T, size_t CAPACITY = 32> // size设定为 2^k
class Ringbuffer {
public:
#define next(x) ((x + 1)&(CAPACITY - 1))
    Ringbuffer() : head_(0), tail_(0) {}

    bool put(const T & value)
    {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t next_head = next(head);
        if (next_head == tail_.load(std::memory_order_acquire))
            return false;
        ring_[head] = value;
        head_.store(next_head, std::memory_order_release);
        return true;
    }
    bool put(T&& value) {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t next_head = next(head);
        if (next_head == tail_.load(std::memory_order_acquire))
            return false;
        ring_[head] = std::move(value);
        head_.store(next_head, std::memory_order_release);
        return true;
    }
    bool get(T & value)
    {
        size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire))
            return false;
        value = ring_[tail];
        tail_.store(next(tail), std::memory_order_release);
        return true;
    }
private:
    T ring_[CAPACITY];
    atomic<size_t> head_, tail_;
};


template<typename T, typename QueueContainer = SafeQueue<T>>
class BlockQueue {
public:
    using Lock = std::unique_lock<std::mutex>;

    void put(const T& x) {
        if(!q.put(x)) {
            Lock lock(p_mtx);
            p_cv.wait(lock, [&]() {return exited_flag || q.put(x);});
        }
        g_cv.notify_one();
    }

    void put(T&& x) {
        if(!q.put(std::move(x))) {
            Lock lock(p_mtx);
            p_cv.wait(lock, [&]() {return exited_flag || q.put(std::move(x));});
        }
        g_cv.notify_one();
    }

    T get() {
        T ret;
        if(!q.get(ret)) {
            Lock lock(g_mtx);
            g_cv.wait(lock, [&]() { return exited_flag || q.get(ret); });
        }
        p_cv.notify_one();
        return ret;
    }

    bool empty() {
        return q.empty();
    }

    size_t size() {
        return q.size();
    }

    void exit() {
        exited_flag = true;
        p_cv.notify_all();
        g_cv.notify_all();
    }

private:
    QueueContainer q;
    std::mutex p_mtx, g_mtx;
    std::condition_variable p_cv, g_cv;
    bool exited_flag = false;
};


#endif //CPPUTILS_SAFE_QUEUE_H
