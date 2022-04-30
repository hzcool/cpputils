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
#include <queue>

class ThreadPool {
public:
    using Functor = std::function<void()>;
    using Lock = std::unique_lock<std::mutex>;

    ThreadPool(unsigned n_threads = 4): _workers(vector<std::thread>(n_threads)), _shutdown(false) {}
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;
    ~ThreadPool() {
        if(!_shutdown) shutdown();
    }

    void start() {
        for(int i = 0; i < _workers.size(); ++i)
            _workers[i] = std::thread(Worker(this, i));
    }

    void shutdown() {
        _shutdown = true;
        _cv.notify_all();
        for(auto &w: _workers)
            if(w.joinable()) w.join();
    }

    template<typename Func = Functor, typename... Args>
    auto submit(Func&& f, Args&&... args) {
        if(_shutdown) throw std::runtime_error("submit task for a stopped thread_pool");
        using WrapperFunc = decltype(f(args...))();
        std::function<WrapperFunc> func = std::bind(std::forward<Func>(f), std::forward<Args>(args)...);
        auto task_ptr = std::make_shared<std::packaged_task<WrapperFunc>>(func);
        Lock lock(_mtx);
        _queue.push([task_ptr]() {(*task_ptr)();});
        _cv.notify_one();
        return task_ptr->get_future();
    }
private:
    struct Worker {
        explicit Worker(ThreadPool* _pool, int _id): pool(_pool), id(_id) {}

        void operator()() {
            auto SZ = pool->_workers.size();
            while(!pool->_shutdown) {
                if(local_queue.empty()) {
                    //从全局队列中获取任务
                    Lock lock(pool->_mtx);
                    if (pool->_queue.empty()) pool->_cv.wait(lock);
                    auto K = (pool->_queue.size() + SZ - 1) / SZ;
                    while(K--) {
                        local_queue.push(std::move(pool->_queue.front()));
                        pool->_queue.pop();
                    }
                    continue;
                }
                Functor f = move(local_queue.front());
                local_queue.pop();
                f();
            }
        }

        std::queue<Functor> local_queue;
        ThreadPool* pool;
        int id;
        friend ThreadPool;
    };


    bool _shutdown;
    vector<std::thread> _workers;
    std::mutex _mtx;
    std::condition_variable _cv;
    std::queue<Functor> _queue;

};
#endif //CPPUTILS_THREAD_POOL_H
