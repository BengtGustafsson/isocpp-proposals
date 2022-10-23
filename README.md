# isocpp-proposals

# My C++ proposals

This repository contains published proposals in separate subdirectories. Some also contains demo implementations, usually
incomplete.

## P2665 Overload Selection by compiler

This proposal allows the compiler to select overload instead of failing when an overload set contains T and const T&. This
allows the compiler to optimize calling convention depending on the T. Example:

```C++
class std::vector<T> {
    void push_back(const T&);
    void push_back(T);
    void push_back(T&&);
};
```

[P2665R0 Markdown]([P2665-OverloadSelection/Published/P2665R0.md]())

[P2665R0 PDF]([isocpp-proposals/P2665-OverloadSelection/Published/P2665R0.pdf]())

[D2665R1 Draft]([isocpp-proposals/P2665-OverloadSelection/D2665R1.md]())

