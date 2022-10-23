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

[P2665R0 Markdown](P2665-OverloadSelection/Published/P2665R0.md)

[P2665R0 PDF](P2665-OverloadSelection/Published/P2665R0.pdf)

[D2665R1 Draft](P2665-OverloadSelection/D2665R1.md)

## P2666 Last Use Optimization

Allow the compiler to treat the last use of a variable (or parameter) as a rvalue automatically. This includes the last use of a rvalue reference parameter and by value data members in rvalue qualified member functions.

A minimum requirement on code structure when the compiler must apply this rvalue conversion is included, but as of yet the exact rule is not specified.

```C++
template<typename T> void func(T&& p)
{
	m_vector.push_back(p);			// No std::forward is needed
}
```

[P2666R0 Markdown](P2666-LastUseOptimization/Published/P2666R0.md)

[P2666R0 PDF](P2666-LastUseOptimization/Published/P2666R0.pdf)

[D2666R1 Draft](P2666-LastUseOptimization/D2666R1.md)

## 
