#pragma once

#include <atomic>
#include <ostream>

// pre-define some useful type
using IntIntTable = rbhash::map<int, int, std::hash<int>, std::equal_to<int>>;

using StringIntTable = rbhash::map<std::string, int, std::hash<std::string>,
    std::equal_to<std::string>>;

// 定义针对std::unique_ptr的特例化，以允许哈希表中存储std::unique_ptr类型的数据
namespace std {
template <typename T>
struct hash<std::unique_ptr<T>> {
    size_t operator()(const std::unique_ptr<T>& ptr) const
    {
        return std::hash<T>{}(*ptr);
    }

    size_t operator()(const T* ptr) const { return std::hash<T>{}(*ptr); }
};

template <typename T>
struct equal_to<std::unique_ptr<T>> {
    bool operator()(const std::unique_ptr<T>& ptr1,
        const std::unique_ptr<T>& ptr2) const
    {
        return *ptr1 == *ptr2;
    }

    bool operator()(T* ptr1, const std::unique_ptr<T>& ptr2) const
    {
        return *ptr1 == *ptr2;
    }

    bool operator()(const std::unique_ptr<T> ptr1, T* ptr2) const
    {
        return *ptr1 == *ptr2;
    }
};
} // namespace std

template <typename T>
using UniquePtrTable = rbhash::map<std::unique_ptr<T>, std::unique_ptr<T>,
    std::hash<std::unique_ptr<T>>,
    std::equal_to<std::unique_ptr<T>>>;

class dummy {
public:
    dummy(int i = 0) noexcept
        : data_(i)
    {
        live.fetch_add(1, std::memory_order_relaxed);
    }
    ~dummy() noexcept { deleted.fetch_add(1, std::memory_order_relaxed); }

    // move constructor
    dummy(dummy&&) noexcept { live.fetch_add(1, std::memory_order_relaxed); }

    dummy& operator=(const dummy&) noexcept
    {
        live.fetch_add(1, std::memory_order_relaxed);
        return *this;
    }

    // 构造对象个数统计
    static std::atomic<uint64_t> live;
    // 已析构对象的个数统计
    static std::atomic<uint64_t> deleted;

private:
    int data_;

    friend std::ostream& operator<<(std::ostream& os, const dummy& d)
    {
        os << d.data_;
        return os;
    }
};

// generateKey is a function from a number to another given type, used to
// generate keys for insertion
template <class T>
T generateKey(size_t i)
{
    return (T)i;
}

// This specialization returns a stringified representation of the given
// integer, where the number is copied to the end of a long string of `a`s, in
// order to make comparisions and hashing take time.
template <>
std::string generateKey<std::string>(size_t n)
{
    const size_t min_length = 100;
    const std::string num(std::to_string(n));
    if (num.size() >= min_length) {
        return num;
    }
    std::string ret(min_length, 'a');
    const size_t startret = min_length - num.size();
    for (size_t i = 0; i < num.size(); ++i) {
        ret[i + startret] = num[i];
    }
    return ret;
}

std::atomic<int64_t>& get_unfreed_bytes();

template <typename T>
struct custom_allocator {
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T value_type;

    custom_allocator() noexcept {}

    template <typename U>
    custom_allocator(const custom_allocator<U>&) noexcept {}

    T* allocate(std::size_t n)
    {
        get_unfreed_bytes() += n * sizeof(T);
        return std::allocator<T>().allocate(n);
    }

    void deallocate(T* p, std::size_t n)
    {
        get_unfreed_bytes() -= n * sizeof(T);
        std::allocator<T>().deallocate(p, n);
    }

    template <typename U, class... Args>
    void construct(U* p, Args&&... args)
    {
        new ((void*)p) U(std::forward<Args>(args)...);
    }

    template <typename U>
    void destroy(U* p)
    {
        p->~U();
    }

    template <typename U>
    struct rebind {
        typedef custom_allocator<U> other;
    };
};

template <typename T, typename U>
inline bool operator==(const custom_allocator<T>& a1,
    const custom_allocator<T>& a2)
{
    return true;
}

template <typename T, typename U>
inline bool operator!=(const custom_allocator<T>& a1,
    const custom_allocator<T>& a2)
{
    return false;
}

using IntIntTableWithCustomAllocator = rbhash::map<int, int, std::hash<int>, std::equal_to<int>,
    custom_allocator<std::pair<const int, int>>>;
