#include <iostream>

#include "timer.h"
#include "thread_pool.h"

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
int main() {
//    TimePoint ed = Clock::now() + MS(20000);
//    Timer t;
//    int i = 0;
//    t.Loop(1000, [&i]() mutable {std::cout << i << "\n"; ++i;});
//    while(Clock::now() <= ed);

    test_thread_pool();
    test_timer();

    return 0;
}
