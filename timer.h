//
// Created by hzcool on 22-4-28.
// 毫秒定时器
//

#ifndef CPPUTILS_TIMER_H
#define CPPUTILS_TIMER_H

#include <thread>
#include <queue>
#include <chrono>
#include <functional>
#include <mutex>
#include "thread_pool.h"

enum EventState {
    Pending, Running, Canceled, Exited
};

using Clock = std::chrono::steady_clock;
using CallBack = std::function<void()>;
using MS = std::chrono::milliseconds;
using TimePoint = std::chrono::time_point<Clock>;
class Event {
public:
    bool cancle() {
        if(!_repeat && _state == Running) return false;
        _state = Canceled;
        return true;
    }
    EventState get_state() {
        return _state;
    }
private:
    explicit Event(const MS& interval, CallBack&& cb,  bool repeat = false): _interval(interval), _next_run(Clock::now() + interval), _cb(std::move(cb)), _repeat(repeat), _state(Pending) {}
    const bool _repeat;
    EventState _state;
    MS _interval;
    TimePoint _next_run;
    CallBack _cb;
    friend class Timer;
};


class Timer {
public:

    using EventPtr = std::shared_ptr<Event>;
    using Lock = std::unique_lock<std::mutex>;

    explicit Timer(int thread_pool_size = 2): _pool(thread_pool_size), _stop(false) {
        _pool.start();
        _monitor = std::thread([this]() { this->start_monitor();});
    }



    ~Timer() {
        if(!_stop) stop();
    }

    template<typename Func, typename... Args>
    EventPtr Once(int64_t delay, Func&& f, Args&&... args) {  // 执行单次
        return addEvent(MS(delay), std::bind(std::forward<Func>(f), std::forward<Args>(args)...));
    }

    template<typename Func, typename... Args>
    EventPtr Loop(int64_t interval, Func&& f, Args&&... args) { //循环执行
        return addEvent(MS(interval), std::bind(std::forward<Func>(f), std::forward<Args>(args)...), true);
    }

    void stop() {
        _stop = true;
        _pool.shutdown();
        _cv.notify_all();
        _monitor.join();
        while(!_events.empty()) {
            EventPtr e = _events.top();
            _events.pop();
            e->cancle();
        }
    }

private:
    void start_monitor() {
        MS sleep_time;
        EventPtr e;
        while (!_stop) {
            {
                Lock lock(_mtx);
                if(_events.empty()) {
                    _cv.wait(lock);
                    if(_stop) break;
                }
                e = _events.top();
                sleep_time = std::chrono::duration_cast<MS>(e->_next_run - Clock::now());
                if(sleep_time.count() > 0) {
                    _cv.wait_for(lock, sleep_time);
                    continue;
                } else _events.pop();
            }
            if(e->_state == Canceled || e->_state == Exited) continue;
            if(e->_state == Pending) e->_state = Running;
            if(!e->_repeat) {
                _pool.submit([e{std::move(e)}](){e->_cb(); e->_state = Exited;});
            } else {
                _pool.submit(e->_cb);
                e->_next_run += e->_interval;
                Lock lock(_mtx);
                _events.push(e);
            }
        }
    }


    EventPtr addEvent(const MS& interval, CallBack&& cb, bool repeat = false) {
        if(_stop) return nullptr;
        EventPtr e = std::make_shared<Event>(Event(interval, std::move(cb), repeat));
        Lock lock(_mtx);
        _events.push(e);
        _cv.notify_one();
        return e;
    }

    struct EventPtrCmp {
        bool operator()(const EventPtr& e1, const EventPtr& e2) {
            return e1->_next_run > e2->_next_run;
        }
    };
    bool _stop;
    std::priority_queue<EventPtr, std::vector<EventPtr>, EventPtrCmp> _events;
    std::mutex _mtx;
    std::condition_variable _cv;
    ThreadPool _pool;
    std::thread _monitor;
};

#endif //CPPUTILS_TIMER_H
