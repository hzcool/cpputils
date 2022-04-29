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
#define LOCAL_QUEUE_SIZE 8

class ThreadPool {
public:
    using Functor = std::function<void()>;
    using Lock = std::unique_lock<std::mutex>;

    ThreadPool(unsigned n_threads = 4): _workers(vector<Worker*>(n_threads)), _shutdown(false)  {}
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;
    ~ThreadPool() {
        if(!_shutdown) shutdown();
    }

    void start() {
        for(auto &w: _workers) {
            w = new Worker(this);
            w->work();
        }
        _dispatch = std::thread([this]() {
            Functor f = block_get(); int SZ = _workers.size();
            while (!_shutdown) {
                int k = _queue.size() / SZ;
                if(k < 1) k = 1;
                for(auto &w: _workers) {
                    for(int i = 0; i < k; ++i) {
                        if(w->que.put(std::move(f))) {
                            if(!_queue.get(f)) {
                                w->cv.notify_one();
                                f = block_get();
                                if(_shutdown) return;
                            }
                        }
                        else break;
                    }
                    w->cv.notify_one();
                }
            }
        });
    }

    void shutdown() {
        _shutdown = true;
        _cv.notify_all();
        if(_dispatch.joinable()) _dispatch.join();
        for(auto &w: _workers) if(w) delete w;
    }

    template<typename Func = Functor, typename... Args>
    auto submit(Func&& f, Args&&... args) {
        if(_shutdown) throw std::runtime_error("submit task for a stopped thread_pool");
        using WrapperFunc = decltype(f(args...))();
        std::function<WrapperFunc> func = std::bind(std::forward<Func>(f), std::forward<Args>(args)...);
        auto task_ptr = std::make_shared<std::packaged_task<WrapperFunc>>(func);
        _queue.put([task_ptr]() {(*task_ptr)();});
        _cv.notify_one();
        return task_ptr->get_future();
    }
private:
    struct Worker {
        explicit Worker(ThreadPool* _pool): pool(_pool) {}
        ~Worker() {
            cv.notify_one();
            if(t.joinable())  t.join();
        }
        void work() {
            t = std::thread([this] {
                Functor f;
                while (!pool->_shutdown) {
                    if(que.get(f)) f();
                    else {
                        Lock lock(mtx);
                        cv.wait(lock);
                    }
                }
            });
        }
        Ringbuffer<Functor, LOCAL_QUEUE_SIZE> que;
        ThreadPool* pool;
        std::condition_variable cv;
        std::mutex mtx;
        std::thread t;

        friend ThreadPool;
    };

    Functor block_get() {
        Functor f;
        if(!_queue.get(f)) {
            Lock lock(_mtx);
            _cv.wait(lock, [&]() {return _shutdown || _queue.get(f);});
        }
        return std::move(f);
    }

    bool _shutdown;
    SafeQueue<Functor> _queue;
    std::mutex _mtx;
    std::condition_variable _cv;

    vector<Worker*> _workers;
    std::thread _dispatch;
};
#endif //CPPUTILS_THREAD_POOL_H
