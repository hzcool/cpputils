//
// Created by hzcool on 22-4-28.
//

#ifndef CPPUTILS_SAFE_QUEUE_H
#define CPPUTILS_SAFE_QUEUE_H

#include <queue>
#include <vector>
#include <mutex>

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

#endif //CPPUTILS_SAFE_QUEUE_H
