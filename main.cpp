#include <bits/stdc++.h>
using namespace std;

#include "timer.h"
#include "thread_pool.h"
#include "safe_queue.h"


void test_thread_pool() {
  ThreadPool pool;
  pool.submit([](){std::cout << "hello world\n";}); //提交任务

  std::future<int> fut = pool.submit([](int x,int y){return x + y;}, 1, 2); // 计算 1 + 2， 通过从返回的 future 获取结果
  int res = fut.get();
  std::cout << res << std::endl;
  pool.shutdown();
}

void test_timer() {
    Timer t;  //默认构造(线程池大小为1)

    t.Once(2000, [](const std::string &s){
      std::cout << s << std::endl;
    }, "hello world"); // 添加单次事件， 2000毫秒后执行一次输出

    int i = 0;
    auto event = t.Loop(1000, [&i]() mutable {
        std::cout << i << std::endl;
        ++i;
    }); // 添加循环事件， 每 1000 毫秒输出自增变量。

    std::this_thread::sleep_for(std::chrono::seconds(10));

    event->get_state(); // 获取事件状态， 枚举类型EventState{Pending, Running, Canceled, Exited}。  未运行时 Pending, 运行中为Running, 任务取消：Canceled， 单次任务才有Exited状态。
    event->cancle(); // 事件取消

    t.stop();  // 终止定时器， 所有未执行的事件都会放弃执行，状态设置为 Canceled。 (未显示调用，析构函数会执行)

}


const unsigned SZ = 20000000;
SignlePVQueue<int, SZ> q;

void P() {
    for(int i = 0; i < SZ; ++i)
        while(!q.put(i));
}

void V() {
    int x;
    for(int i = 0; i < SZ; ++i)
        while(!q.get(x));
}


SafeQueue<int> qq;
void P2() {
    for(int i = 0; i < SZ; ++i)
        qq.put(i);
}

void V2() {
    int x;
    for(int i = 0; i < SZ; ++i)
        while(!qq.get(x));
}



int main() {

    auto st = Clock::now();
    thread t1(P);
    thread t2(V);
    t1.join(); t2.join();
    cout << chrono::duration_cast<MS>(Clock::now() - st).count() / 1000.0 << "\n";
    cout << q.empty() << endl;

//    st = Clock::now();
//    thread t3(P2);
//    thread t4(V2);
//    t3.join(); t4.join();
//    cout << chrono::duration_cast<MS>(Clock::now() - st).count() / 1000.0 << "\n";
//    cout << qq.empty() << endl;
    return 0;
}
