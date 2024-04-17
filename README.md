# isocpp-proposals

# My C++ proposals

This repository contains published proposals in separate subdirectories. Some also contains demo implementations, usually
incomplete, intended only to prove the point.

In addition to these proposals I have also been quite heavily involved in the proposal for *universal template parameters* P1985 and have had discussions with the authors of the static_vector and polymorphic_value proposals regarding the applicability of P2667. I also wrote a proposal P0565 for pack indexing and constexpr for loops a number of years ago, but never followed up on it.

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

[D2665R1 Draft](D2665R1.md)

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

[D2666R1 Draft](D2666R1.md)

## P2667 Vector Allocator for SBO

This proposal contains changes to allocator and vector to allow vector to work as a static_vector and sbo_vector. It implements this as `std::buffered_allocator` and `std::throwing_allocator` etc for static_vector building. It also contains an extension of vector construction and assignment to include source vectors with same value_type but different allocators.

The first version R0 implements this as a few new traits, proposing to place these along with the `allocate_at_least` function in a new namespace `allocator_info` which is to replace or complement `allocator_traits`.

Synopsis:

```C++
namespace std {
  namespace allocator_info {
    template<typename A> bool can_allocate = true;
    template<typename A> size_t buffer_capacity = 0;
    template<typename A> using backing_allocator_of = A;
  }

  template<typename T, size_t SZ, typename Backing> class buffered_allocator;
  template<typename T> class throwing_allocator;
  template<typename T> class terminating_allocator;
  template<typename T> class unchecked_allocator;
}
```

An alternative would be to provide different concepts and specialize vector depending on which concepts are modelled by the actual allocator type instead of relying on new traits to modify the vector implementation.

[P2667R0 Markdown](P2667-VectorAllocatorForSBO/Published/P2667R0.md)

[P2667R0 PDF](P2667-VectorAllocatorForSBO/Published/P2667R0.pdf)

[D2667R1 Draft](D2667R1.md)

## P2668 Role Based Parameter Passing

This proposal adds a new *kind* called **type_set** which is used as a way to generate closed sets of function signatures from one function declaration or definition. Type sets are usually templates and a special rule similar to that of concepts allows the generation of sets of different cvref qualifications for the same basic type. In addition using a type set as a parameter type for templates disables universal reference processing which allows precise control over which overloads get generated for function templates.

This example also assumes P2665 and P2666:

```C++
template<typename P> type_set fwd = <const P, const P&, P&&>;  // Thanks to P2665

template<typename T> class vector {
    void push_back(fwd T value) { m_data[m_size++] = value; }
};
```

The one written push_back function generates three signatures according to the definition of **fwd**. When writing `fwd T` the T is used as the first template parameter of fwd as for concepts.

Note that in the assignment `value` does not have to be written `forward<decltype(value)>(value)` thanks to P2666.

[P2668R0 Markdown](P2668-RoleBasedParameterPassing/Published/P2668R0.md)

[P2668R0 PDF](P2668-RoleBasedParameterPassing/Published/P2668R0.pdf)

[D2668R1 Draft](D2668R1.md)

## P2669 Deprecate Kind Change

This proposal deprecates and ultimately forbids changing the kind of a name between the definition of a class template and its explicit specializations, for names declared in more than one. This includes the case that there is no definition but more than one explicit specialization (full or partial).

This allows template code to rely on knowing the kind of names in a class template for which it has seen the definition. This removes the need for about 75 % of current disambiguations using `typename`.

It would also make a get member function for tuple and variant more palatable as it would never have to be disambiguated using `template.`

[P2669R0 Markdown](P2669-DeprecateKindChange/Published/P2669R0.md)

[P2669R0 PDF](P2669-DeprecateKindChange/Published/P2669R0.pdf)

[D2669R1 Draft](D2669R1.md)

## P3183 Contract testing support

This proposal contains a few magic functions which can run only pre and post conditions of a function with a set of parameters. This is primarily useful to implement unit tests for these conditions, which is otherwise tricky as the program terminates if a condition fails.

[P3183R0 Markdown](P3183-ContractTestingSupport/Published/P3183R0.md)

[P3183R0 PDF](P3183-ContractTestingSupport/Published/P3183R0.pdf)

[D3183R1 Draft](D3183R1.md)
