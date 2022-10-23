#pragma once

/// This file contains additions to the <memory> header proposed in P2667. In addition to this it is proposed to
/// move the allocate_at_least function here, but for the moment it forwards to std::

#include <memory>

namespace std {

// Stolen from MS STL version not even in VS preview.
template <class _Ptr>
struct allocation_result {
    _Ptr ptr;
    size_t count;
};

template <class _Alloc>
concept _Has_member_allocate_at_least = requires(_Alloc _Al, size_t _Count) {
    _Al.allocate_at_least(_Count);
};

template <class _Alloc>
constexpr allocation_result<typename allocator_traits<_Alloc>::pointer> allocate_at_least(
    _Alloc& _Al, const size_t _Count) {
    if constexpr (_Has_member_allocate_at_least<_Alloc>) {
        return _Al.allocate_at_least(_Count);
    } else {
        return {_Al.allocate(_Count), _Count};
    }
}

namespace allocator_info {
namespace detail {

    // Handle that some Alloc's don't have these values
    template<typename Alloc> constexpr typename Alloc::size_type __get_buffer_capacity() {
        if constexpr ( requires { Alloc::buffer_capacity; })
            return Alloc::buffer_capacity;
        else
            return 0;
    }

    
    // Handle that some Alloc's don't have these values
    template<typename Alloc> constexpr bool __get_can_allocate() {
        if constexpr ( requires { Alloc::can_allocate; })
            return Alloc::can_allocate;
        else
            return true;
    }

}  // namespace detail


template<typename Alloc> struct backing_allocator_of {
    using type = Alloc;
};

template<typename Alloc> using backing_allocator_of_t = backing_allocator_of<Alloc>::type;

template<typename Alloc> constexpr typename Alloc::size_type buffer_capacity = detail::__get_buffer_capacity<Alloc>();
template<typename Alloc> constexpr bool can_allocate = detail::__get_can_allocate<Alloc>();

// Forwarding to std:: namespace version. Hopefully that function is moved here before C++23 is finalized.
template<typename Alloc> auto allocate_at_least(Alloc& allocator, typename Alloc::size_type sz) { return std::allocate_at_least(allocator, sz); }

}  // namespace allocator_info


template<typename T, size_t SZ, typename Backing = allocator<T>> class buffered_allocator {
    using Traits = allocator_traits<Backing>;
public:
    // As the baccking allocator can have some special ideas about this we must forward all of them through Traits.
    using value_type = Traits::value_type;
    using pointer = Traits::pointer;
    using const_pointer = Traits::const_pointer;
    using void_pointer = Traits::void_pointer;
    using const_void_pointer = Traits::const_void_pointer;
    using difference_type = Traits::difference_type;
    using size_type = Traits::size_type;

    struct propagate_on_container_copy_assignment : Traits::propagate_on_container_copy_assignment {};
    struct propagate_on_container_move_assignment : Traits::propagate_on_container_move_assignment {};
    struct propagate_on_container_swap : Traits::propagate_on_container_swap {};

    // New info
    static const size_type buffer_capacity = SZ;
    static constexpr bool can_allocate = allocator_info::can_allocate<Backing>;

    template<typename U, typename... Args> struct rebind {
        using other = buffered_allocator<U, SZ, typename Traits::template rebind_alloc<U, Args...>>;
    };

    // Forward all constructor parameters to backing except when contstructed from a buffered_allocator of different SZ
    // This includes construction directly from a Backing allocator.
    template<typename... Args> buffered_allocator(Args&&... args) : m_backingAllocator(forward<Args>(args)...) {}

    // Construct by move/copy of the source backing allocator if it matches, and T matches.
    template<size_t SZ> buffered_allocator(const buffered_allocator<T, SZ, Backing>& src) : m_backingAllocator(src.m_backingAllocator) {} 
    template<size_t SZ> buffered_allocator(buffered_allocator<T, SZ, Backing>&& src) : m_backingAllocator(move(src.m_backingAllocator)) {} 

    // Convert to backing allocator at will, as these don't construct from me.
    operator Backing&& () && { return move(m_backingAllocator); }
    operator const Backing& () const & { return m_backingAllocator; }

    T* allocate(size_type count) { return reinterpret_cast<T*>(m_data); }
    void deallocate(T* p, size_type count) { 
        if (p == allocate(0))
            return;

        Traits::deallocate(m_backingAllocator, p, count);
    }

    allocation_result<pointer> allocate_at_least(size_type count) {
        if (count <= SZ)
            return { allocate(0), SZ };
        else
            return std::allocate_at_least(m_backingAllocator, count);
    }
    
    constexpr size_type max_size() { return max(SZ, Traits::max_size()); }       // If the backing allocator returns 0 return SZ.

    template<class T, class... Args> constexpr void construct(T* p, Args&&... args) {
        Traits::construct(m_backingAllocator, p, forward<Args>(args)...);
    }
    
    void destroy(T* p) { 
        if (p != allocate(0))
            Traits::destroy(m_backingAllocator, p); 
    }

    template<size_t SZR, typename Backing> friend bool operator==(const buffered_allocator& lhs, const buffered_allocator<T, SZR, Backing>& rhs) { return lhs.m_backingAllocator == rhs.m_backingAllocator; }
    template<size_t SZR, typename Backing> friend bool operator==(const buffered_allocator& lhs, const Backing& rhs) { return lhs.m_backingAllocator == rhs; }

private:
    byte m_data[SZ * sizeof(T)];
    [[no_unique_address]] Backing m_backingAllocator;
};


namespace allocator_info {

// Partially specialize backing_allocator_of for buffered_allocator to reveal its backing allocator type.
template<typename T, size_t SZ, typename Alloc> struct backing_allocator_of<buffered_allocator<T, SZ, Alloc>> {
    using type = backing_allocator_of_t<Alloc>;       // Recurse if necessary
};

}  // namespace allocator_info


template<typename T> struct terminating_allocator {
    using value_type = T;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    constexpr static bool can_allocate = false;

    T* allocate(size_type count) { terminate(); }
    void deallocate(T*, size_type) {}

    static constexpr size_type max_size() { return 0; }
    constexpr bool operator==(const terminating_allocator&) { return true; }
};


template<typename T> struct throwing_allocator {
    using value_type = T;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    constexpr static bool can_allocate = false;

    T* allocate(size_type count) { throw bad_alloc(); }
    void deallocate(T*, size_type) {}

    static constexpr size_type max_size() { return 0; }
    constexpr bool operator==(const throwing_allocator&) { return true; }
};


template<typename T> struct unchecked_allocator {
    using value_type = T;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    constexpr static bool can_allocate = false;

    T* allocate(size_type count) { return nullptr; }
    void deallocate(T*, size_type) {}

    static constexpr size_type max_size() { return 0; }
    constexpr bool operator==(const unchecked_allocator&) { return true; }
};


}  // namespace std

