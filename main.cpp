#include <bits/stdc++.h>
using namespace std;

#include "timer.h"
#include "thread_pool.h"
#include "safe_queue.h"
#include "traits.h"


void test_thread_pool() {
  ThreadPool pool;
  pool.start();
//  pool.submit([](){std::cout << "hello world\n";}); //提交任务
//
//  std::future<int> fut = pool.submit([](int x,int y){return x + y;}, 1, 2); // 计算 1 + 2， 通过从返回的 future 获取结果
//  int res = fut.get();
//  std::cout << res << std::endl;
    int SZ = 10000;
    atomic<int> sum = 0;
    array<thread, 10> a;
    for(auto &t : a) {
        t = thread([&]() {
            for(int i = 0; i < SZ; ++i)
                pool.submit([&sum]() {sum.fetch_add(1);});
        });
    }
    for(auto &t : a) t.join();
  this_thread::sleep_for(std::chrono::seconds(2));
  cout << sum << "\n";
  pool.shutdown();
}

void test_timer() {
    Timer t;  //默认构造(线程池大小为2)

    t.Once(2000, [](const std::string &s){
      std::cout << s << std::endl;
    }, "hello world"); // 添加单次事件， 2000毫秒后执行一次输出

    int i = 0;
    auto event = t.Loop(1000, [&i]() mutable {
        std::cout << i << std::endl;
        ++i;
    }); // 添加循环事件， 每 1000 毫秒输出自增变量。

    std::this_thread::sleep_for(std::chrono::seconds(11));

    event->get_state(); // 获取事件状态， 枚举类型EventState{Pending, Running, Canceled, Exited}。  未运行时 Pending, 运行中为Running, 任务取消：Canceled， 单次任务才有Exited状态。
    event->cancle(); // 事件取消

    t.stop();  // 终止定时器， 所有未执行的事件都会放弃执行，状态设置为 Canceled。 (未显示调用，析构函数会执行)
}

//void test_block_queue() {
//    SPSCBlockQueue<int, 10> q;
//
//    thread t1([&]() {
//        for(int i = 0; i < 10000; ++i) {
//            q.put(i);
//        }
//    });
//
//    thread t2([&]() {
//        int cnt = 0;
//        for(int i = 0; i < 10000; ++i) {
//            cnt += q.get();
//        }
//        cout << cnt << endl;
//    });
//
//    t1.join();
//    t2.join();
//}



void fuck(int a, string b, int c, bool d, char t) {

}

template <typename Tp>
void TypeJudge() {
    if constexpr(is_same_v<Tp, int>) {
        cout << "int\n";
    } else if constexpr(is_same_v<Tp, string>) {
        cout << "string\n";
    } else if constexpr(is_same_v<Tp, bool>) {
        cout << "bool\n";
    } else {
        cout << "other\n";
    }
}


using H = FunctionTrait<decltype(fuck)>;

template<size_t i>
void check() {
    if constexpr(i >= 1)  {
        TypeJudge<typename H::arg<i - 1>::type>();
        check<i - 1>();
    }
}

int main() {

    test_context();

    return 0;
}
