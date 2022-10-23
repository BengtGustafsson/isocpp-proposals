#pragma once

// Include memory and the new traits in the 
#include "experimental_memory.h"

#include <type_traits>
#include <tuple>

namespace std {

namespace detail {
template<size_t SZ> auto __get_uint_holding()
{
    if constexpr (SZ < 256)
        return uint8_t();
    else if constexpr (SZ < 65536)
        return uint16_t();
    else if constexpr (SZ < 65536*65536ull)
        return uint32_t();
    else
        return uint64_t();
}


// Possible to standardize but not in this proposal, hence in detail::
template<size_t SZ> using uint_holding = decltype(detail::__get_uint_holding<SZ>());

// For static vectors (!can_allocate) just one int of appropriate size.
template<typename T, size_t SZ> struct vector_storage {
    uint_holding<SZ> m_size;
};

// The semantics and layout here is the same as in older vector implementation of the same std library implementation to
// preserve ABI compatibility.
template<typename T> struct vector_storage<T, 0> {
    T* m_begin = nullptr;
    T* m_end = nullptr;
    T* m_capacity = nullptr;
};

}  // namespace detail


// Stripped down vector with just a few methods, but which shows what needs to be done in vector to implement P2667
template<typename T, typename Alloc = allocator<T>> class vector {
public:
    using value_type = T;
    using size_type = size_t;
    template<typename T, typename > friend class vector;

private:
    static const size_type buffer_capacity = allocator_info::buffer_capacity<Alloc>;
    static const bool can_allocate = allocator_info::can_allocate<Alloc>;
    using Traits = std::allocator_traits<Alloc>;
    using Backing = allocator_info::backing_allocator_of_t<Alloc>;

public:
    vector() {
        if constexpr (can_allocate) {
            m_storage.m_begin = nullptr;
            m_storage.m_end = nullptr;
            m_storage.m_capacity = nullptr;
        }
        else
            m_storage.m_size = 0;
    }

    template<typename A> vector(vector<T, A>&& src) {   // operator= works the same but definitely has to handle propagate_on_container_move_assignment
        using BA = allocator_info::backing_allocator_of_t<A>;
        if constexpr (is_same_v<Backing, BA> && allocator_info::can_allocate<A>) {
            if (src.capacity() > allocator_info::buffer_capacity<A> || src.size() > buffer_capacity) { // src has allocated, and I would have to allocate
                m_alloc = std::move(src.m_alloc);   // Unclear if this should be conditioned on propagate_on_container_move_assignment in move ctor.

                // move three pointers here, clear source's pointers.
                m_storage = move(src.m_storage);
                src.m_storage.m_begin = nullptr;
                src.m_storage.m_end = nullptr;
                src.m_storage.m_capacity = nullptr;
                return;
            }
        }

        take_elements(src.data(), src.size());
        src.clear();   // Always leave source empty even if it had to be copied.
    }
    template<typename A> vector(const vector<T, A>& src) {
        copy_elements(src.data(), src.size());
    }

    ~vector() { destroy_me(); }

    template<typename A> vector& operator=(vector<T, A>&& src) {
        using BA = allocator_info::backing_allocator_of_t<A>;
        if constexpr (is_same_v<Backing, BA> && allocator_info::can_allocate<A>) {
            if (src.capacity() > allocator_info::buffer_capacity<A> && src.size() > buffer_capacity) {  // src has allocated, and I will have to allocate
                if (std::allocator_traits<Backing>::propagate_on_container_move_assignment::value || allocator_traits<Backing>::is_always_equal::value || m_alloc == src.m_alloc) {  // And taking the src element vector is allowed
                    destroy_me();
                    if constexpr (std::allocator_traits<Backing>::propagate_on_container_move_assignment::value)
                        m_alloc = std::move(src.m_alloc);  // Unclear if this should happen even if !propagate_on_container_move_assignment.

                    // move three pointers here, clear source's pointers.
                    m_storage = move(src.m_storage);
                    src.m_storage.m_begin = nullptr;
                    src.m_storage.m_end = nullptr;
                    src.m_storage.m_capacity = nullptr;
                    return *this;   // Local return to avoid multiple else branches below.
                }
            }
        }

        take_elements(src.data(), src.size());
        src.clear();   // Always leave source empty even if it had to be copied.
        return *this;
    }

    template<typename A> vector& operator=(const vector<T, A>& src) {
        using BA = allocator_info::backing_allocator_of_t<A>;
        // CppReference writes that the allocator copy should happen first and then discuesses what to do if the allocators _would
        // have_ compared unequal. I don't understand how that is not the same as the code here, barring possibly an exception
        // during the m_alloc copy operation. This is highly unpractical as destroy_me uses the set m_alloc at the time (which must
        // be the old one). To fulfill the writing one would have to save the copy of the old allocator somewhere and then _that_
        // copying could throw anyway.
        if constexpr (is_same_v<Backing, BA> && std::allocator_traits<Backing>::propagate_on_container_copy_assignment::value) {
            if (!allocator_traits<Backing>::is_always_equal::value && m_alloc != src.m_alloc)
                destroy_me();

            m_alloc = src.m_alloc;
        }

        copy_elements(src.data(), src.size());
    }


    // Mini-API to show what needs to be changed. All other operations are easily implemented in terms of these.
    size_type capacity() const {
        if constexpr (can_allocate)
            return m_storage.m_capacity - m_storage.m_begin;
        else
            return allocator_info::buffer_capacity<Alloc>;
    }
    T* data() {
        if constexpr (can_allocate)
            return m_storage.m_begin;
        else
            return m_alloc.allocate(0);
    }
    size_type size() const {
        if constexpr (can_allocate)
            return m_storage.m_end - m_storage.m_begin;
        else
            return m_storage.m_size;
    }
    bool empty() const { return size() == 0; }

    void push_back(const T& elem) {
        bump(size() + 1);
        Traits::construct(m_alloc, end(), elem);
        set_size(size() + 1);
    }
    void pop_back() {
        set_size(size() - 1);
        Traits::destroy(m_alloc, end());
    }

    void reserve(size_type sz) {
        if (sz <= capacity())
            return;

        if constexpr (can_allocate) {
            auto result = allocator_info::allocate_at_least(m_alloc, sz);

            size_type old_size = size();

            // Move the data, deallocate old buffer... as today
            for (size_type i = 0; i < old_size; i++)
                Traits::construct(m_alloc, result.ptr + i, move(operator[](i)));

            Traits::deallocate(m_alloc, data(), capacity());

            m_storage.m_begin = result.ptr;
            m_storage.m_capacity = result.ptr + result.count;
            m_storage.m_end = result.ptr + old_size;
        }
        else
            // This throws or terminates or something if size too large. As we don't use the return value there is a fair chance
            // that the if inside buffered_allocator is optimized away if the called backing_allocator returns nullptr always.
            (void) allocator_info::allocate_at_least(m_alloc, sz);
    }

    void resize(size_type sz) {
        if (sz > size()) {
            bump(sz);
            while (size() < sz)
                push_back({});
        }
        else {
            while (size() > sz)
                pop_back();
        }
    }
    void clear() { resize(0); }

    // Used by tests
    T& operator[](size_t ix) { return data()[ix]; }
    T* begin() { return data(); }
    T* end() { return begin() + size(); }

private:
    void destroy_me() {
        clear();
        Traits::deallocate(m_alloc, data(), capacity());
    }
    void bump(size_type sz) {
        if (sz <= size())
            return;

        reserve(max(sz, size() * 3 / 2));
    }

    void set_size(size_type sz) {
        if constexpr (can_allocate)
            m_storage.m_end = m_storage.m_begin + sz;
        else
            m_storage.m_size = detail::uint_holding<buffer_capacity>(sz);
    }

    void take_elements(T* src, size_type count) {
        reserve(count);

        T* dest = data();

        if (count > size()) {
            // First move-assign for as many elements I already have
            dest = move(src, src + size(), dest);
            // Construct the rest
            for (size_type i = size(); i < count; i++) {
                Traits::construct(m_alloc, dest, move(src[i]));
                dest++;
            }
        }
        else {
            // First move-assign for as many elements as src has
            dest = move(src, src + count, dest);

            // Destroy the rest
            for (size_type i = count; i < size(); i++) {
                Traits::destroy(m_alloc, dest);
                dest++;
            }
        }

        set_size(count);
    }
    void copy_elements(T* src, size_type count) {
        reserve(count);

        T* dest = data();

        if (count > size()) {
            // First copy-assign for as many elements I already have
            dest = copy(src, src + size(), dest);

            // Construct the rest
            for (size_type i = size(); i < count; i++) {
                Traits::construct(m_alloc, dest, src[i]);
                dest++;
            }
        }
        else {
            // First copy-assign for as many elements I already have
            dest = copy(src, src + count, dest);

            // Destroy the rest
            for (size_type i = count; i < size(); i++) {
                Traits::destroy(m_alloc, dest);
                dest++;
            }
        }

        set_size(count);
    }

    // Order between storage and allocator preserved.
    detail::vector_storage<T, can_allocate ? 0 : buffer_capacity> m_storage;
    [[no_unique_address]] Alloc m_alloc;        // No need for empty base optimization anymore.
};


template<typename T, size_t SZ, typename Backing = std::allocator<T>> 
    using sbo_vector = vector<T, buffered_allocator<T, SZ, Backing>>;

template<typename T, size_t SZ>
using static_vector_throw = sbo_vector<T, SZ, throwing_allocator<T>>;

template<typename T, size_t SZ>
using static_vector_terminate = sbo_vector<T, SZ, throwing_allocator<T>>;

template<typename T, size_t SZ>
using static_vector = sbo_vector<T, SZ, unchecked_allocator<T>>;

} // namespace std
