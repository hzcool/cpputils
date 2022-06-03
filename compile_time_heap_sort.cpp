//
// Created by hzcool on 22-6-3.
//

#include <bits/stdc++.h>
using namespace std;

// 编译时期堆排序
template<typename T, T... args>
struct List{
    static constexpr int size = sizeof...(args);
};

//合并若干 List
template<typename T, typename... Lists>
struct Merge {
    using result = List<T>;
};
template<typename T, T... head, typename... Lists>
struct Merge<T, List<T, head...>, Lists...> {
    using result = List<T, head...>;
};
template<typename T, T... head, T... tail, typename... Lists>
struct Merge<T, List<T, head...>, List<T, tail...>, Lists...>  {
    using result = typename Merge<T, List<T, head..., tail...>, Lists...>::result;
};

// 分裂提取 List 的第 n 个元素， List => (HeadList, nth_ele, TailList)
template<typename T, int n, typename Q>
struct SplitImpl {};
template<typename T, T first, T... args>
struct SplitImpl<T, 0, List<T, first, args...>> {
    using Head = List<T>;
    static constexpr T ele = first;
    using Tail = List<T, args...>;
};
template<typename T, int n, T first, T... args>
struct SplitImpl<T, n, List<T, first, args...>> {
    using TmpResult = SplitImpl<T, n - 1, List<T, args...>>;
    using Head = typename Merge<T, List<T, first>, typename TmpResult::Head>::result;
    static constexpr T ele = TmpResult::ele;
    using Tail = typename TmpResult::Tail;
};
template<typename T, int n, T... args>
struct Split: public SplitImpl<T, n, List<T, args...>> {};


// 交换 List 第idx0, idx1元素, bool 为 false 不交换
template<typename T, int idx0, int idx1, typename Q, bool>
struct SwapImpl{};
template<typename T, int idx0, int idx1, T... args>
struct SwapImpl<T, idx0, idx1, List<T, args...>, true>{
    using lst = List<T, args...>;
    using A = SplitImpl<T, idx0, lst>;
    using B = SplitImpl<T, idx1 - idx0 - 1, typename  A::Tail>;
    using result = typename Merge<T, typename A::Head, List<T, B::ele>, typename B::Head, List<T, A::ele>, typename B::Tail >::result;
};
template<typename T, int idx0, int idx1, T... args>
struct SwapImpl<T, idx0, idx1, List<T, args...>, false> {
    using result = List<T, args...>;
};
template<typename T, int idx0, int idx1, T... args>  //idx0 < idx1
struct Swap: public SwapImpl<T, idx0, idx1, List<T, args...>, idx0 < idx1 && idx0 >=0 && idx1 < List<T, args...>::size> {};

template<typename Cmp>
struct ReverseCmp{
    template<typename T>
    constexpr bool operator()(const T& x, const T& y) const{
        return !Cmp{}(x, y);
    }
};
template<typename T, typename Cmp, int n, bool le, bool re, typename Q>

struct HeapDownImpl{};
template<typename T, typename Cmp, int n, T... args>
struct HeapDownImpl<T, Cmp, n, false, false, List<T, args...>> {
    using result = List<T, args...>;
};
template<typename T, typename Cmp, int n,  T... args>
struct HeapDownImpl<T, Cmp, n, true, false, List<T, args...>> {
    using result = typename SwapImpl<T, n, 2 * n + 1, List<T, args...>, !Cmp{}(Split<T, n, args...>::ele , Split<T, 2 * n + 1, args...>::ele) >::result;
};
template<typename T, typename Cmp, int n, T... args>
struct HeapDownImpl<T, Cmp, n, true, true, List<T, args...>> {
    using lst =  List<T, args...>;
    static constexpr int _idx = Cmp{}(Split<T, n, args...>::ele , Split<T, 2 * n + 1, args...>::ele) ? n : 2 * n + 1;
    static constexpr int idx = Cmp{}(Split<T, _idx, args...>::ele , Split<T, 2 * n + 2, args...>::ele) ? _idx : 2 * n + 2;
    using tmp = typename SwapImpl<T, n, idx, lst, n != idx>::result;
    using result = typename HeapDownImpl<T, Cmp, idx, idx != n &&  2 * idx + 1 < lst::size,  idx != n && 2 * idx + 2 < lst::size, tmp>::result;
};
template<typename T, typename Cmp, int n,  T... args>
struct HeapDown: public HeapDownImpl<T, Cmp, n, 2 * n + 1 < sizeof...(args), 2 * n + 2 < sizeof...(args), List<T, args...>> {};

template<typename T, typename Cmp, int n, typename Q>
struct MakeHeapImpl{};
template<typename T, typename Cmp, T... args>
struct MakeHeapImpl<T, Cmp, -1, List<T, args...>> {
    using result = List<T, args...>;
};
template<typename T, typename Cmp, T... args>
struct MakeHeapImpl<T, Cmp, 0, List<T, args...>> {
    using result = typename HeapDown<T, Cmp, 0, args...>::result;
};
template<typename T, typename Cmp, int n, T... args>
struct MakeHeapImpl<T, Cmp, n, List<T, args...>> {
    using result = typename MakeHeapImpl<T, Cmp, n - 1, typename HeapDown<T, Cmp, n, args...>::result>::result;
};
template<typename T, typename Cmp, T... args>
struct MakeHeap: public MakeHeapImpl<T, Cmp, sizeof...(args) / 2 - 1, List<T, args...>> {};

template<typename T, typename Cmp, typename P, typename Q>
struct HeapSortImpl{};
template<typename T, typename Cmp, T... sorted>
struct HeapSortImpl<T, Cmp, List<T>, List<T, sorted...>> {
    using result = List<T, sorted...>;
};
template<typename T, typename Cmp, T... args, T... sorted>
struct HeapSortImpl<T, Cmp, List<T, args...>, List<T, sorted...>> {
    static constexpr int n = sizeof...(args) - 1;
    using tmp = SplitImpl<T, n, typename Swap<T, 0, n, args...>::result>;
    using result = typename HeapSortImpl<T, Cmp, typename MakeHeapImpl<T, ReverseCmp<Cmp>, 0, typename tmp::Head>::result, typename Merge<T, List<T, tmp::ele>, List<T, sorted...>>::result >::result;
};
template<typename T, typename Cmp, T... args>
struct HeapSort {
    using result = typename HeapSortImpl<T, Cmp, typename MakeHeap<T, ReverseCmp<Cmp>, args...>::result, List<T>>::result;
};

template<typename T>
void ptln(List<T>) {}

template<typename T, T first, T... args>
void ptln(List<T, first, args...>) {
    cout << first;
    if constexpr(sizeof...(args) == 0) cout << "\n";
    else {
        cout << " ";
        ptln(List<T, args...>{});
    }
}

int main()
{
    using t = HeapSort<int, less<>, -100, 9, 8, 7, 4, 100, -2, 2, 1, 0>::result;
    ptln(t{});
    return 0;
}