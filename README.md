# isocpp-proposals

# My own C++ proposals with text and any reference implementation

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
