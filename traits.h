//
// Created by hzcool on 22-5-1.
//

#ifndef CPPUTILS_TRAITS_H
#define CPPUTILS_TRAITS_H


#include <cstdlib>
#include <tuple>

template<typename R>
struct FunctionTrait {};

template<typename R, typename... Args>
struct FunctionTrait<std::function<R(Args...)>> { //std::function
    static const size_t nargs = sizeof...(Args);
    using ReturnType = R;
    template<size_t i>
    struct arg {
        using type = std::tuple_element_t<i, std::tuple<Args...>>;
    };
};

template<typename R, typename... Args>
struct FunctionTrait<R(Args...)>:  FunctionTrait<std::function<R(Args...)>> {  // 普通函数
};

template<typename T>
struct lambda_wrapper {
    using type = decltype(std::function(*T{}));
};

template<typename Lambda>
struct FunctionTrait<lambda_wrapper<Lambda>> : FunctionTrait<typename lambda_wrapper<Lambda>::type>{  // lambda
};


/*
 Context可以用于 http 的 handle 处理链。
 1. 在bind_arg绑定 handle 的各个参数， 比如请求的 Query 或 Form 数据， 也可以绑定Context自己
 2. Context.nxt(); 进入下一个handle，  context.abort()， 当前的handle执行完不进入后面的handle
*/

class Context {
public:
    using Functor = std::function<void()>;

    template<typename T>
    T bind_arg() {
        if constexpr(is_same_v<T, int>) return T{1};
        if constexpr(is_same_v<T, string>) return T{"hello world"};
        if constexpr(is_same_v<T, Context*>) return this;
        if constexpr(is_same_v<T, Context&>) return *this;
    }

    template<typename ResType, typename... Args>
    void add(std::function<ResType(Args...)> &&f) {
        auto func = bind(move(f), (bind_arg<Args>())...);
        lst.push_back([func{move(func)}](){func();});
    }

    template<typename ResType, typename... Args> //function
    void add(const std::function<ResType(Args...)> &f) {
        auto func = bind(f, (bind_arg<Args>())...);
        lst.push_back([func{move(func)}](){func();});
    }

    template<typename ResType, typename... Args>
    void add(ResType(&f)(Args...)) {  //普通函数
        auto func = bind(move(f), (bind_arg<Args>())...);
        lst.push_back([func{move(func)}](){func();});
    }

    template<typename F>
    void add(F&& f) { // lambda
        add(function(f));
    }

    template<typename T, typename... Args>
    void add(T&& first, Args&&... args) {
        add(std::forward<T>(first));
        add(std::forward<Args>(args)...);
    }


    void nxt() {
        ++cur;
        work();
    }

    void abort() {
        _abort = true;
    }

    void work() {
        if(cur + 1 > lst.size()) return;
        int tmp = cur;
        lst[cur]();
        if(_abort || tmp != cur) return;
        nxt();
    }

    vector<Functor> lst;
    int cur;
    bool _abort;
};

void f1() {
    cout << "f1 start\n";
    cout << "f1 ed\n";
}

void f2(Context* c) {
    cout << "f2 start\n";
    c->nxt();
    cout << "f2 ed\n";
}

void f3(Context* c) {
    cout << "f3 start\n";
    c->nxt();
    cout << "f3 ed\n";
}

void f4(Context* c) {
    cout << "f4 start\n";
    cout << "f4 ed\n";
    c->abort();
}

void f5(Context* c) {
    cout << "f5 start\n";
    cout << "f5 ed\n";
}

void test_context() {
    Context *c = new Context;
    c->add(f1, f2, f3, std::function(f4), f5, [](int x, int y) {
        cout << x + y << "\n";
    });
    c->work();
}

#endif //CPPUTILS_TRAITS_H
