### C++ 安全队列， 线程池， 定时器


#### 1. 安全队列
```C++
SafeQueue<int> q; //先进先出线程安安全队列
SafePriorityQueue<int> pq; //线程安全优先队列
```


#### 2. 线程池
线程池创建时自动生产所有的线程
```C++
ThreadPool pool; // 默认 4 个线程的线程池
ThreadPool pool2(10); // 10 个线程的线程池

std::future<int> fut = pool.submit([](int x,int y){return x + y;}, 1, 2); // 计算 1 + 2， 通过从返回的 future 获取结果
int res = fut.get();
std::cout << res << std::endl;
pool.shutdown(); // 关闭线程池
```

#### 3. 定时器
可以定义时间单位为毫秒的的单次或循环事件,  事件执行利用线程池

```C++
Timer t2(10); //构造初始线程池大小为 10 的定时器

Timer t;  //默认构造(线程池大小为1)

t.Once(2000, [](const std::string &s){
    std::cout << s << std::endl;
}, "hello world"); // 添加单次事件， 2000 毫秒后执行输出

int i = 0;
auto event = t.Loop(1000, [&i]() mutable {
    std::cout << i << std::endl;
    ++i;
}); // 添加循环事件， 每 1000 毫秒输出自增变量。

std::this_thread::sleep_for(std::chrono::seconds(10));

event->get_state(); // 获取事件状态， 枚举类型EventState{Pending, Running, Canceled, Exited}。  未运行时 Pending, 运行中为Running, 任务取消：Canceled， 单次任务才有Exited状态。
event->cancle(); // 事件取消

t.stop();  // 终止定时器， 所有未执行的事件都会放弃执行，状态设置为 Canceled。 (未显示调用，析构函数会执行)
```



