//
// Created by hzcool on 22-4-28.
//

#ifndef CPPUTILS_THREAD_POOL_H
#define CPPUTILS_THREAD_POOL_H

#include <functional>
#include <future>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "safe_queue.h"

class ThreadPool {
public:
    using Functor = std::function<void()>;
    using Lock = std::unique_lock<std::mutex>;

    ThreadPool(const int n_threads = 4): _threads(std::vector<std::thread>(n_threads)) ,_shutdown(false) {
        for(int i = 0; i < _threads.size(); ++i) {
            _threads[i] = std::thread(worker, i);
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;
    ~ThreadPool() { if(!_shutdown) shutdown();}

    void shutdown() {
        _shutdown = true;
        _cv.notify_all();
        for(auto &t : _threads) t.join();
    }

    template<typename Func = Functor, typename... Args>
    auto submit(Func&& f, Args&&... args) {
        using WrapperFunc = decltype(f(args...))();
        std::function<WrapperFunc> func = std::bind(std::forward<Func>(f), std::forward<Args>(args)...);
        auto task_ptr = std::make_shared<std::packaged_task<WrapperFunc>>(func);
        _queue.put([task_ptr]() {(*task_ptr)();});
        _cv.notify_one(); //唤醒一个线程
        return task_ptr->get_future();
    }
private:
    bool _shutdown;
    SafeQueue<Functor> _queue;
    std::vector<std::thread> _threads;
    std::mutex _conditional_mutex;
    std::condition_variable _cv;
    std::function<void(int)> worker = [this](int id) {
        Functor f;
        bool exist;
        while (!_shutdown) {
            {
                Lock lock(_conditional_mutex);
                if(_queue.empty()) _cv.wait(lock);
                exist = _queue.get(f);
            }
            if(exist) f();
        }
    };
};
#endif //CPPUTILS_THREAD_POOL_H
