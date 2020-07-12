// Copyright (c) 2020 The rbhash Authors. All rights reserved.

#pragma once

#include <assert.h>
#include <stdlib.h>

#include <atomic>
#include <deque>
#include <exception>
#include <functional>
#include <limits>
#include <list>
#include <mutex>
#include <new>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace rbhash {

/**
 * @brief 定义counter类型，spinlock使用此类型计数其负责的元素个数
 */
using counter_type = int64_t;

/**
 * @brief 哈希表默认初始化大小（大小为2的HASHMAP_DEFAULT_HASHPOWER次幂）
 */
#define HASHMAP_DEFAULT_HASHPOWER 16U

/**
 * @brief 哈希表进行扩容时可自主启动的线程数目
 */
#define HASHMAP_MAX_EXTRA_WORKER 8U

/**
 * @brief 使用C++11 atomic库中atomic_flag实现的自旋锁，哈希表内部使用
 *
 * @details
 * spinlock会记录它保护的元素个数以及一个标志位，用于标识该spinlock负责保护的元素
 *          在哈希表扩容时是否完成了迁移；id无实际用途，仅供debug时使用
 */
class alignas(64) spinlock_t {
 public:
  /**
   * @brief 构造spinlock_t对象，默认为解锁状态
   */
  spinlock_t() : element_counter_(0), is_migrated_(true) { lock_.clear(); }

  /**
   * @brief 拷贝构造一个新的spinlock_t对象，默认为解锁状态
   *
   * @param other spinlock_t类型的对象引用
   */
  spinlock_t(const spinlock_t& other)
      : element_counter_(other.elem_counter()),
        is_migrated_(other.is_migrated()) {
    lock_.clear();
  }

  /**
   * @brief 赋值操作符实现
   *
   * @param other spinlock_t类型的对象引用
   * @return spinlock_t& 赋值之后的spinlock对象
   */
  spinlock_t& operator=(const spinlock_t& other) {
    elem_counter() = other.elem_counter();
    is_migrated() = other.is_migrated();
    return *this;
  }

  /**
   * @brief 加锁操作，直到成功才返回，否则自旋
   */
  void lock() noexcept {
    while (lock_.test_and_set(std::memory_order_acq_rel))
      ;
  }

  /**
   * @brief 解锁操作
   * @pre spinlock必须处于加锁状态
   */
  void unlock() noexcept { lock_.clear(std::memory_order_release); }

  /**
   * @brief 尝试加锁操作
   *
   * @return true 加锁成功
   * @return false 加锁失败
   */
  bool try_lock() noexcept {
    return !lock_.test_and_set(std::memory_order_acq_rel);
  }

  /**
   * @brief 获取spinlock负责的元素计数变量的左值引用（可读可写）
   *
   * @return counter_type& 计数变量引用
   */
  counter_type& elem_counter() noexcept { return element_counter_; }

  /**
   * @brief 获取spinlock负责的元素计数变量的值（可读不可写）
   *
   * @return counter_type 计数变量值
   */
  counter_type elem_counter() const noexcept { return element_counter_; }

  /**
   * @brief 获取spinlock迁移标志位变量的左值引用（可读可写）
   *
   * @return true 迁移完成
   * @return false 迁移未完成
   */
  bool& is_migrated() noexcept { return is_migrated_; }

  /**
   * @brief 获取spinlock迁移标志（可读不可写）
   *
   * @return true 迁移完成
   * @return false 迁移未完成
   */
  bool is_migrated() const noexcept { return is_migrated_; }

 private:
  std::atomic_flag lock_;
  counter_type element_counter_;
  bool is_migrated_;
};

/**
 * @brief 哈希表中使用Table作为底层存储来保存所有键值对，本质上是一个数组
 *
 * @tparam Key 哈希表中存储的键（key）类型
 * @tparam Value 哈希表中存储的值（value）类型
 */
template <typename Key, typename Value, class Allocator>
class table {
 private:
  using traits_ =
      typename std::allocator_traits<Allocator>::template rebind_traits<Value>;

 public:
  /// 和标准库类似，定义key_type为Key的别名
  using key_type = Key;
  /// 和标准库类似，定义mapped_type为Value的别名
  using mapped_type = Value;
  /// 和标准库类似，定义value_type为键值对的聚合
  using value_type = std::pair<const Key, Value>;
  /// 和标准库类似，定义给reference类型
  using reference = value_type&;
  /// 和标准库类似，定义const reference类型
  using const_reference = const value_type&;
  /// 和标准库类似，定义pointer类型
  using pointer = value_type*;
  /// 和标准库类似，定义const pointer类型
  using const_pointer = const value_type*;
  /// 和标准库类似，定义size_type类型
  using size_type = std::size_t;
  /// 和标准库类似，定义allocator_type类型
  using allocator_type = typename traits_::allocator_type;

  /// bucket是用于保存键值对的容器（一个bucket目前仅保存一对键值）
  class bucket {
   public:
    /**
     * @brief 构造一个bucket，内部状态全部初始化为默认值
     *
     */
    bucket() noexcept : occupied_(false), deleted_(false) {}

    /// 获取键值对的const左值引用
    const value_type& kvpair() const {
      return *static_cast<const value_type*>(
          static_cast<const void*>(&storage_));
    }
    /// 获取键值对的左值引用，一般用于赋值
    value_type& kvpair() {
      return *static_cast<value_type*>(static_cast<void*>(&storage_));
    }

    /// 获取Key的const左值引用
    const key_type& key() const { return storage_kvpair().first; }
    /// 获取Key的右值引用
    key_type&& movable_key() { return std::move(storage_kvpair().first); }

    /// 获取Value的const左值引用
    const mapped_type& mapped() const { return storage_kvpair().second; }
    /// 获取Value的左值引用，一般用于赋值
    mapped_type& mapped() { return storage_kvpair().second; }
    /// 获取Value的右值引用，一般用于赋值
    mapped_type&& movable_mapped() {
      return std::move(storage_kvpair().second);
    }

    /// 获取占用标志位的左值引用，一般用于赋值
    bool& occupied() { return occupied_; }
    /// 获取占用标志位的当前值
    bool occupied() const { return occupied_; }

    /// 获取删除标志位的左值引用，一般用于赋值
    bool& deleted() { return deleted_; }
    /// 获取删除标志位的当前值
    bool deleted() const { return deleted_; }

   private:
    friend class table;

    /// 定义底层使用的具体存储类型，注意和value_type不同
    using storage_value_type = std::pair<Key, Value>;
    /// 获取底层实际使用存储类型的const左值引用
    const storage_value_type& storage_kvpair() const {
      return *static_cast<const storage_value_type*>(
          static_cast<const void*>(&storage_));
    }
    /// 获取底层实际使用的存储类型的左值引用，一般用于赋值
    storage_value_type& storage_kvpair() {
      return *static_cast<storage_value_type*>(static_cast<void*>(&storage_));
    }

    /// 满足底层存储结构对齐要求的内存空间
    typename std::aligned_storage<sizeof(storage_value_type),
                                  alignof(storage_value_type)>::type storage_;
    /// 被占用标志位
    bool occupied_;
    /// 删除标志位
    bool deleted_;
    /// hash value
    size_type hash_value_;
  };

 public:
  table() = default;

  /**
   * @brief 构造Table对象
   *
   * @param hp hashpower哈希表的参数透传到Table，用于申请对应大小的内存空间；
   *           Table构造时将申请2的hashpower幂的内存用于保存键值对
   */
  table(uint64_t hp, const allocator_type& allocator = Allocator())
      : hashpower_(hp),
        allocator_(allocator),
        bucket_allocator_(allocator_),
        buckets_(bucket_allocator_.allocate(size())) {
    assert(buckets_ != nullptr);
    static_assert(std::is_nothrow_constructible<bucket>::value,
                  "table requires bucket to be nothrow constructible");
    for (int i = 0; i < size(); ++i) {
      traits_::construct(allocator_, &buckets_[i]);
    }
  }

  /**
   * @brief 移动构造
   *
   * @param other 待移动的其他Table资源；移动构造之后，other变为初始化的状态
   */
  table(table&& other)
      : hashpower_(other.hashpower()),
        allocator_(std::move(other.allocator_)),
        bucket_allocator_(allocator_),
        buckets_(std::move(other.buckets_)) {
    other.buckets_ = nullptr;
    other.hashpower(0);
  }

  /**
   * @brief Table移动赋值操作符
   *
   * @param other 待移动的其他Table；移动赋值之后，other变为初始化的状态
   */
  table& operator=(table&& other) {
    if (this != &other) {
      destroy_buckets();
      hashpower(other.hashpower());
      buckets_ = other.buckets_;
      allocator_ = std::move(other.allocator_);
      bucket_allocator_ = allocator_;
      other.hashpower(0);
      other.buckets_ = nullptr;
    }
    return *this;
  }

  /**
   * @brief 销毁Table对象，释放保存的所有数据
   */
  ~table() noexcept { destroy_buckets(); }

  /**
   * @brief 和其他Table交换内部状态和数据
   *
   * @param other 待交换的其他Table
   */
  void swap(table& other) noexcept {
    swap_allocator(allocator_, other.allocator_,
                   typename traits_::propagate_on_container_swap());
    swap_allocator(bucket_allocator_, other.bucket_allocator_,
                   typename traits_::propagate_on_container_swap());

    size_t other_hashpower = other.hashpower();
    other.hashpower(hashpower());
    hashpower(other_hashpower);
    std::swap(buckets_, other.buckets_);
  }

  /// 返回当前hashpower值
  size_type hashpower() const {
    return hashpower_.load(std::memory_order_acquire);
  }

  /// 设置新的hashpower值
  void hashpower(size_type val) {
    hashpower_.store(val, std::memory_order_release);
  }

  /// 获取Table的当前大小
  size_type size() const { return static_cast<size_type>(1) << hashpower(); }

  /// Table支持下标操作，下标操作符实现
  bucket& operator[](size_type i) { return buckets_[i]; }
  const bucket& operator[](size_type i) const { return buckets_[i]; }

  /// 在ind指向的bucket中构造键值对，值部分支持可变长参数进行构造
  template <typename K, typename... Args>
  void setKV(size_type ind, K&& k, Args&&... args) {
    bucket& b = buckets_[ind];
    assert(!b.occupied() || b.deleted());
    traits_::construct(allocator_, std::addressof(b.storage_kvpair()),
                       std::piecewise_construct,
                       std::forward_as_tuple(std::forward<K>(k)),
                       std::forward_as_tuple(std::forward<Args>(args)...));

    // This must occur last, to enforce a strong exception guarantee
    b.occupied() = true;
    b.deleted() = false;
  }

  /// 销毁（析构但不释放内存）table中ind指向的bucket中的数据，但并不清除occupied标志位，而设置deleted标志位
  void eraseKV(size_type ind) {
    bucket& b = buckets_[ind];
    assert(b.occupied());
    b.deleted() = true;
    traits_::destroy(allocator_, std::addressof(b.storage_kvpair()));
  }

  /// 销毁Table中的所有数据，并释放bucket所占用的内存空间
  void clear_and_deallocate() noexcept { destroy_buckets(); }

  /// 销毁Table中的所有数据，但并不释放bucket占用的内存空间
  void clear() noexcept {
    static_assert(std::is_nothrow_destructible<key_type>::value &&
                      std::is_nothrow_destructible<mapped_type>::value,
                  "table requires key and value to be nothrow destructible");
    if (buckets_ == nullptr) return;
    for (size_type i = 0; i < size(); ++i) {
      bucket& b = buckets_[i];
      if (b.occupied() && !b.deleted()) {
        eraseKV(i);
      }
      b.occupied() = false;
    }
  }

  /// 辅助函数，销毁buckets保存的数据并释放buckets内存空间
  void destroy_buckets() noexcept {
    if (buckets_ == nullptr) {
      return;
    }
    static_assert(std::is_nothrow_destructible<bucket>::value,
                  "table requires bucket to be nothrow destructible");
    clear();
    for (size_type i = 0; i < size(); ++i) {
      traits_::destroy(allocator_, &buckets_[i]);
    }
    bucket_allocator_.deallocate(buckets_, size());
    buckets_ = nullptr;
  }

  /// 返回占用的内存大小，字节数
  size_t footprint() const { return sizeof(bucket) * size(); }

 private:
  template <typename A>
  void swap_allocator(A& dst, A& src, std::true_type) {
    std::swap(dst, src);
  }

  template <typename A>
  void swap_allocator(A&, A&, std::false_type) {}

  std::atomic<size_t> hashpower_;

  allocator_type allocator_;

  typename traits_::template rebind_alloc<bucket> bucket_allocator_;

  bucket* buckets_ = nullptr;
};

/**
 * @brief
 * 并发线程安全的哈希表实现，使用线性探测法解决哈希冲突；插入冲突时，将自动触发哈希表的扩容（linear_rehash()）
 *        支持哈希表的收缩，但不会自动触发，需要显式的调用收缩接口（shrink()）
 *        哈希表必须处于锁定状态才允许迭代操作
 *
 * @tparam Key 哈希表中存储的键类型
 * @tparam Value 哈希表中存储的值类型
 * @tparam Hash 哈希函数，默认使用std::hash<Key>
 * @tparam KeyEqual 判断Key是否相等的相等函数，默认使用std::equal_to<Key>
 * @tparam Allocator 自定义Allocator，默认使用std::allocator<std::pair<const
 * Key, Value>>
 * @see linear_rehash()
 * @see shrink()
 */
template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator = std::allocator<std::pair<const Key, Value>>>
class map {
 public:
  /// 定义buckets_t类型为Table类型别名
  using buckets_t = table<Key, Value, Allocator>;
  /// 和标准库类似，定义key_type，这里直接使用Table中的定义
  using key_type = typename buckets_t::key_type;
  /// 和标准库类似，定义mapped_type，这里直接使用Table中的定义
  using mapped_type = typename buckets_t::mapped_type;
  /// 和标准库类似，定义value_type，这里直接使用Table中定义
  using value_type = typename buckets_t::value_type;
  /// 定义size_type，这里直接使用Table中的定义
  using size_type = typename buckets_t::size_type;
  /// 和标准库类似，这里直接使用Table中的定义
  using reference = typename buckets_t::reference;
  /// 和标准库类似，这里直接使用Table中的定义
  using const_reference = typename buckets_t::const_reference;
  /// 和标准库类似，这里直接使用Table中的定义
  using pointer = typename buckets_t::pointer;
  /// 和标准库类似，这里直接使用Table中的定义
  using const_pointer = typename buckets_t::const_pointer;
  /// 和标准库类似，这里直接使用Table中的定义
  using allocator_type = typename buckets_t::allocator_type;
  /// 和标准库类似，定义迭代器差值类型
  using difference_type = std::ptrdiff_t;
  /// 定义hasher为模板参数Hash函数的别名
  using hasher = Hash;
  /// 定义key_equal为模板参数KeyEqual函数的别名
  using key_equal = KeyEqual;

  /// 前向声明locked_table类型，表示锁定状态的哈希表（用于迭代器实现）
  class locked_table;

  /**
   * @brief 构造给定容量的rb_hashmap，初始状态为空
   *
   * @param hp （hashpower）哈希表的初始容量（容量大小为2^hashpower）
   * @param hf 哈希函数
   * @param eq_f 相等函数，用于判断key值是否相等
   */
  map(size_type hp = HASHMAP_DEFAULT_HASHPOWER, const Hash& hf = Hash(),
      const KeyEqual& eq_f = KeyEqual(), const Allocator& alloc = Allocator())
      : hash_fn_(hf),
        eq_fn_(eq_f),
        buckets_(hp, alloc),
        old_buckets_(),
        max_num_worker_threads_(HASHMAP_MAX_EXTRA_WORKER) {
    all_locks_.emplace_back(std::min(bucket_count(), size_type(kMaxNumLocks)));
  }

  /**
   * @brief
   * 从迭代器构造指定容量的rb_hashmap，如果指定容量不足以容纳迭代器区间
   *        的元素，rb_hashmap会进行自动扩容
   *
   * @tparam InputIt 迭代器类型，可以通过it->first, it->second访问到key和value
   * @param first 起始迭代器
   * @param last 终止迭代器（不包含），遵循常见的左开右闭约定
   * @param hp （hashpower）哈希表的初始容量（容量为2的hashpower次幂）
   * @param hf 哈希函数
   * @param eq_f 相等函数，用于判断两个key值是否相等
   */
  template <typename InputIt>
  map(InputIt first, InputIt last, size_type hp = HASHMAP_DEFAULT_HASHPOWER,
      const Hash& hf = Hash(), const KeyEqual& eq_f = KeyEqual(),
      const Allocator& alloc = Allocator())
      : map(hp, hf, eq_f, alloc) {
    for (; first != last; ++first) {
      insert(first->first, first->second);
    }
  }

  /**
   * @brief 移动构造函数
   *
   * @param other 待移动的其他rb_hashmap的右值引用
   */
  map(map&& other)
      : hash_fn_(std::move(other.hash_fn_)),
        eq_fn_(std::move(other.eq_fn_)),
        buckets_(std::move(other.buckets_)),
        old_buckets_(std::move(other.old_buckets_)),
        max_num_worker_threads_(other.max_num_worker_threads()),
        all_locks_(std::move(other.all_locks_)) {}

  /**
   * @brief 从初始化列表构造一个rb_hashmap
   *
   * @param init 初始化列表
   * @param hp （hashpower）哈希表的初始容量（容量大小为2^hashpower）
   * @param hf 哈希函数
   * @param eq_f 相等函数，用于判断key值是否相等
   */
  map(std::initializer_list<value_type> init,
      size_type hp = HASHMAP_DEFAULT_HASHPOWER, const Hash& hf = Hash(),
      const KeyEqual& eq_f = KeyEqual(), const Allocator& alloc = Allocator())
      : map(init.begin(), init.end(), hp, hf, eq_f, alloc) {}

  /**
   * @brief rb_hashmap不支持拷贝构造和拷贝赋值
   */
  map(map const&) = delete;
  map& operator=(map const&) = delete;

  /**
   * @brief 析构函数
   */
  ~map() { clear_and_free(); }

  template <typename U>
  using rebind_alloc =
      typename std::allocator_traits<allocator_type>::template rebind_alloc<U>;

  /// 自旋锁集合类型，此集合包含哈希表当前时刻拥有的所有自旋锁
  using locks_t = std::vector<spinlock_t, rebind_alloc<spinlock_t>>;
  /// 历史上所有的和当前自旋锁集合的列表，按照时间线组成一条链表（注意历史上的自旋锁集合并不会删除）
  using all_locks_t = std::list<locks_t, rebind_alloc<locks_t>>;

  /// 定义了允许的最大自旋锁集合大小
  static constexpr size_type kMaxNumLocks = 1UL << 16;

  /// 获取当前时刻哈希表拥有的自旋锁集合
  locks_t& get_current_locks() const { return all_locks_.back(); }

  /// 获取哈希函数
  hasher hash_function() const { return hash_fn_; }

  /// 获取key比较函数
  key_equal keq_eq() const { return eq_fn_; }

  /// 获取hashpower
  size_type hashpower() const { return buckets_.hashpower(); }

  /// 获取用于保存键值对的内部bucket的数量
  size_type bucket_count() const { return buckets_.size(); }

  /// 判断哈希表当前是否为空
  bool empty() const { return size() == 0; }

  /// 获取哈希表的当前大小
  size_type size() const {
    if (all_locks_.size() == 0) {
      return 0;
    }
    counter_type s = 0;
    for (spinlock_t& lock : get_current_locks()) {
      s += lock.elem_counter();
    }
    assert(s >= 0);
    return static_cast<size_type>(s);
  }

  /// 获取哈希表的容量
  size_type capacity() const { return bucket_count(); }

  /// 估算哈希表占用内存大小，字节数
  size_type footprint() const {
    size_type lock_cnt = 0;
    for (auto& locks : all_locks_) {
      lock_cnt += locks.size();
    }

    size_type stack = 0;
    return lock_cnt * sizeof(spinlock_t) + stack + buckets_.footprint() +
           old_buckets_.footprint();
  }

  /// 获取哈希表的负载情况
  double load_factor() const {
    return static_cast<double>(size()) / static_cast<double>(capacity());
  }

  /// 设置哈希表扩容时允许启动的额外线程数
  void max_num_worker_threads(size_type extra_threads) {
    max_num_worker_threads_.store(extra_threads, std::memory_order_release);
  }

  /// 获取哈希表扩容时允许启动的额外线程数的当前设置
  size_type max_num_worker_threads() const {
    return max_num_worker_threads_.load(std::memory_order_acquire);
  }

  /**
   * @brief Key-Value插入操作的API接口
   *
   * @tparam K 待插入键（Key）的类型
   * @tparam Args 可变参模板参数，用于构造Value
   * @param key 待插入的具体键（key）
   * @param val 用于构造value的参数
   * @return true 插入哈希表成功
   * @return false 插入哈希表失败
   *
   * @note 如果待插入的元素已经存在，则插入失败，返回false
   * @see insert_or_assign
   */
  template <typename K, typename... Args>
  bool insert(K&& key, Args&&... val) {
    return upsert(std::forward<K>(key), [](mapped_type&) {},
                  std::forward<Args>(val)...);
  }

  /**
   * @brief
   * 哈希表插入或者修改API接口，如果key不存在，则插入；如果存在，则对value部分进行赋值
   *
   * @see insert()
   */
  template <typename K, typename V>
  bool insert_or_assign(K&& key, V&& val) {
    return upsert(std::forward<K>(key), [&val](mapped_type& m) { m = val; },
                  std::forward<V>(val));
  }

  /**
   * @brief Key-Value查找的API接口
   *
   * @tparam K 待查找的键（Key）类型
   * @param key 待查找的具体键（key）值
   * @param val 如果key在哈希表中，键key所关联的value值
   * @return true key存在于哈希表中
   * @return false key不在哈希表中
   * @see update_fn()
   */
  template <typename K>
  bool find(const K& key, mapped_type& val) const {
    return find_fn(key, [&val](const mapped_type& v) mutable { val = v; });
  }

  /**
   * @brief Key-Value查找的API接口
   *
   * @tparam K 待查找的键（Key）类型
   * @param key 待查找的具体键（key）
   * @return mapped_type 在表中key所关联的value值
   * @note 如果key不存在表中，则会抛std::out_of_range异常
   */
  template <typename K>
  mapped_type find(const K& key) {
    const hash_value hv = hashed_key(key);
    table_position pos = linear_find_loop(key, hv);
    if (pos.status == ok) {
      return buckets_[pos.index].mapped();
    } else {
      throw std::out_of_range("key not found");
    }
  }

  /**
   * @brief Key-Value更新的API接口
   *
   * @tparam K 待更新的键（Key）类型
   * @tparam V 待更新的值（Value）类型
   * @param key 待更新的具体键（key）
   * @param val 待更新的具体值（value）
   * @return true 更新操作成功
   * @return false 更新操作失败
   * @see update_fn()
   */
  template <typename K, typename V>
  bool update(const K& key, V&& val) {
    return update_fn(key, [&val](mapped_type& v) { v = std::forward<V>(val); });
  }

  /**
   * @brief
   * 更新/插入API接口函数；通过uprase_fn辅助函数实现，其中传递uprase_fn的函数固定返回false
   *
   * @tparam K 待更新/插入键（Key）的类型
   * @tparam F 对key关联的值进行处理的函数类型
   * @tparam Args 可变参模板参数，用于构造Value
   * @param key 待插入的具体键（key）
   * @param fn 对key关联的值进行处理的具体函数
   * @param val 用于构造value的参数
   * @return true 更新或插入成功
   * @return false 更新或插入失败
   * @see uprase_fn
   */
  template <typename K, typename F, typename... Args>
  bool upsert(K&& key, F fn, Args&&... val) {
    return uprase_fn(std::forward<K>(key),
                     [&fn](mapped_type& v) {
                       fn(v);
                       return false;
                     },
                     std::forward<Args>(val)...);
  }

  /**
   * @brief 删除Key的API接口
   *
   * @tparam K 待删除的键（Key）类型
   * @param key 待删除的具体键（key）
   * @return true 删除成功
   * @return false 删除失败
   * @see erase_fn
   */
  template <typename K>
  bool erase(const K& key) {
    return erase_fn(key, [](mapped_type&) { return true; });
  }

  /// 哈希表容量收缩API接口
  void shrink() {
    while (load_factor() <= 1 / 4) {
      const size_type hp = hashpower();
      if (hp > 1) {
        linear_expand(hp, hp - 1);
        continue;
      }
      break;
    }
  }

  /// 哈希表rehash API接口
  bool rehash(size_type hp) { return linear_rehash(hp); }

  /// 哈希表reserve接口，预留能容纳n个key-value对的内存空间
  bool reserve(size_type n) { return linear_reserve(n); }

  /**
   * @brief 清空哈希表的API接口函数，不释放内存
   * @see linear_clear()
   */
  void clear() {
    auto all_locks_manager = lock_all();
    if (all_locks_manager) {
      ++nr_clear;
      linear_clear();
    }
  }

  /**
   * @brief 清空哈希表的API接口函数，释放内存
   * @see linear_free()
   */
  void clear_and_free() {
    auto all_locks_manager = lock_all();
    if (all_locks_manager) {
      ++nr_clear;
      linear_free();
    }
  }

  /// 从this指针构造locked_table，哈希表将处于被锁住的状态
  locked_table lock_table() { return locked_table(*this); }

  /**
   * @brief      获取统计指标
   *
   * @return     返回 json 格式的统计指标字符串
   */
  std::string stat() {
    // TODO
    return "";
  }

  /**
   * @brief 查找API的辅助函数
   *
   * @tparam K 待查找的键（Key）类型
   * @tparam F 对key所关联的value进行操作的类型
   * @param key 待查找的具体键（key）
   * @param fn 对key所关联的value所进行的操作
   * @return true key在哈希表中，执行fn指定的操作
   * @return false key不在哈希表中
   * @see find()
   * @see update()
   */
  template <typename K, typename F>
  bool find_fn(const K& key, F fn) const {
    const hash_value hv = hashed_key(key);
    table_position pos = linear_find_loop(key, hv);
    if (pos.status == ok) {
      fn(buckets_[pos.index].mapped());
      return true;
    } else {
      return false;
    }
  }

  /**
   * @brief 更新API的辅助函数
   *
   * @see find_fn()
   */
  template <typename K, typename F>
  bool update_fn(const K& key, F fn) const {
    return find_fn(key, fn);
  }

 private:
  /// 用于spinlock_t 智能指针（unique_ptr）的删除器
  struct LockDeleter {
    void operator()(spinlock_t* l) const { l->unlock(); }
  };

  /// spinlock_t的智能指针定义
  using LockManager = std::unique_ptr<spinlock_t, LockDeleter>;

  /// 通过bucket_ind获取对应的spinlock的索引，即负责管理该bucket的spinlock
  inline size_type lock_ind(const size_type bucket_ind) const {
    return bucket_ind & (get_current_locks().size() - 1);
  }

  /**
   * @brief 对哈希表进行操作的错误码，仅内部使用外部不可见
   */
  enum op_status {
    /// 操作正常完成
    ok,
    /// 操作失败
    failure,
    /// 找不到对应的Key
    failure_key_not_found,
    /// Key重复（key已经存在于哈希表中）
    failure_key_duplicated,
    /// 哈希表正在扩容中
    failure_under_expansion
  };

  /// 一个复合类型，用于某些函数的返回值，需要一个表位置的索引以及一个错误码的集合
  struct table_position {
    size_type index;
    op_status status;
    LockManager lock;
  };

  /// 对key进行哈希之后的哈希值
  struct hash_value {
    size_type hash;
  };

  /// 指向rb_hashmap的智能指针(std::unique_ptr)的删除器
  struct AllUnlocker {
    void operator()(map* map) const {
      for (auto it = first_locked; it != map->all_locks_.end(); ++it) {
        locks_t& locks = *it;
        for (spinlock_t& lock : locks) {
          lock.unlock();
        }
      }
    }
    typename all_locks_t::iterator first_locked;
  };

  /// 定义指向rb_hashmap的智能指针（std::unique_ptr）
  using AllLocksManager = std::unique_ptr<map, AllUnlocker>;

  /// 空类型，仅内部使用；该类型会被当异常类型抛出，作为当前哈希表正在进行扩容的信号
  class hashpower_changed {};

  /**
   * @brief 检查哈希表当前是否正处于扩容过程中
   *
   * @param hp 之前记录的hashpower
   * @param lock 管理某个bucket的spinlock
   * @note 判断哈希表是否正在扩容的标志是hashpower前后是否一致：如果不一致，
   *       则表示哈希表正处于扩容中，此时解锁lock并抛hashpower_changed异常；如果一致，
   *       则表示哈希表处于稳定状态；哈希表扩容时hashpower会加1，哈希表容量为原来的2倍
   * @pre lock处于被锁定的状态
   */
  inline void check_hashpower(size_type hp, spinlock_t& lock) const {
    if (hashpower() != hp) {
      lock.unlock();
      throw hashpower_changed();
    }
  }

  /**
   * @brief 尝试对特定的bucket上锁
   *
   * @param hp 加锁前拿到的hashpower
   * @param i bucket的索引值
   * @return spinlock_t* 指向自旋锁的指针（该锁处于锁定的状态）
   */
  spinlock_t* lock_one(size_type hp, size_type i) const {
    locks_t& locks = get_current_locks();
    const size_type l = lock_ind(i);
    assert(l < kMaxNumLocks);
    spinlock_t& lock = locks[l];
    lock.lock();
    assert(!lock.try_lock());
    check_hashpower(hp, lock);
    return &lock;
  }

  /**
   * @brief 将指定的bucket锁住，loop的含义是处理lock_one()可能的失败情况，
   *        在循环体中执行lock_one，直到获得特定bucket的访问权限
   *
   * @param hp 加锁前拿到的hashpower
   * @param ind bucket的索引值
   * @param retry_counter 重试次数；如果发生哈希表扩容则会被复位
   * @param hv 对Key进行哈希得到的哈希值
   * @return LockManager 对lock_one返回的自旋锁构造的智能指针（unique_ptr）
   * @see lock_one()
   */
  LockManager lock_one_loop(size_type& hp, size_type& ind,
                            size_type& retry_counter,
                            const hash_value& hv) const {
    while (true) {
      try {
        spinlock_t* lock = lock_one(hp, ind);
        return LockManager(lock);
      } catch (hashpower_changed&) {
        // The hashpower changed while taking the locks. Try again.
        hp = hashpower();
        ind = index_hash(hp, hv.hash);
        retry_counter = 0;
      }
    }
  }

  /**
   * @brief 将哈希表所有的自旋锁都加锁，获取哈希表的唯一访问权限
   *
   * @return AllLocksManager 指向哈希表的智能指针（std::unique_ptr）
   * @see AllLocksManager
   */
  AllLocksManager lock_all() {
    // all_locks_ should never decrease in size, so if it is non-empty now, it
    // will remain non-empty
    if (all_locks_.empty()) return {};
    const auto first_locked = std::prev(all_locks_.end());
    auto current_locks = first_locked;
    while (current_locks != all_locks_.end()) {
      locks_t& locks = *current_locks;
      for (spinlock_t& lock : locks) {
        lock.lock();
      }
      ++current_locks;
    }
    // Once we have taken all the locks of the "current" container, nobody
    // else can do locking operations on the table.
    return AllLocksManager(this, AllUnlocker{first_locked});
  }

  /**
   * @brief 线性探测查找函数（内部使用）
   *
   * @tparam K 待查找的键（Key）类型
   * @param key 待查找的键（key）值
   * @param hv 待查找key值的哈希值
   * @return table_position 返回的查找结果，包含位置信息和错误码以及对应的自旋锁
   * @see table_position
   */
  template <typename K>
  table_position linear_find_loop(const K& key, const hash_value& hv) const {
    size_type retry_counter = 0, hp = hashpower();
    size_type ind = index_hash(hp, hv.hash);
    while (true) {
      // retry_counter will be reset when hashtable is under expansion
      LockManager lock = lock_one_loop(hp, ind, retry_counter, hv);
      auto& b = buckets_[ind];
      if (!b.occupied()) {
        return {0, failure_key_not_found, nullptr};
      } else if (b.deleted()) {
        // deleted flag act as tombstone
      } else if (keq_eq()(b.key(), key)) {
        return {ind, ok, std::move(lock)};
      }
      // worst case of linear search
      if (++retry_counter >= hp) {
        break;
      }
      ind = index_hash(hp, ++ind);
    }
    return {0, failure_key_not_found, nullptr};
  }

  /**
   * @brief 在哈希表被锁住（调用过lock_all()）的情况下，进行的线性查找
   *
   * @tparam K 待查找的键（Key）类型
   * @param key 待查找的具体键（key）值
   * @return table_position
   * 返回查找的结果；因为表处于被上锁的状态，因此结果中的锁总是为空
   * @see lock_all()
   * @see linear_find_loop()
   */
  template <typename K>
  table_position locked_linear_find_loop(const K& key) const {
    const hash_value hv = hashed_key(key);
    size_type retry_counter = 0, hp = hashpower();
    size_type ind = index_hash(hp, hv.hash);
    while (true) {
      auto& b = buckets_[ind];
      if (!b.occupied()) {
        return {0, failure_key_not_found, nullptr};
      } else if (b.deleted()) {
        // deleted flag act as tombstone
      } else if (keq_eq()(b.key(), key)) {
        return {ind, ok, nullptr};
      }
      // worst case of linear search
      if (++retry_counter >= hp) {
        break;
      }
      ind = index_hash(hp, ++ind);
    }
    return {0, failure_key_not_found, nullptr};
  }

  /**
   * @brief 使用线性探测法对哈希表进行插入操作的辅助函数
   *
   * @tparam K 待插入键的类型
   * @param key 待插入的具体键（key）
   * @param hv 对key进行哈希之后的哈希值
   * @return table_position 返回可以在表中进行插入的位置
   */
  template <typename K>
  table_position linear_insert_loop(K const& key, const hash_value& hv) {
    size_type retry_counter = 0;
    size_type hp = hashpower();
    size_type ind = index_hash(hp, hv.hash);
    while (true) {
      LockManager lock = lock_one_loop(hp, ind, retry_counter, hv);
      assert(!lock->try_lock());
      auto& b = buckets_[ind];
      if (!b.occupied() || b.deleted()) {
        return {ind, ok, std::move(lock)};
      } else if (keq_eq()(b.key(), key)) {
        return {ind, failure_key_duplicated, std::move(lock)};
      }
      ind = index_hash(hp, ++ind);
      if (++retry_counter >= hp) {
        lock.reset();
        linear_expand(hp, hp + 1);
        hp = hashpower();
        ind = index_hash(hp, hv.hash);
        retry_counter = 0;
      }
    }
    return {0, failure, nullptr};
  }

  /**
   * @brief 更新/插入/删除操作真正的实现，如果fn返回true则删除指定的键值对
   *
   * @see upsert()
   */
  template <typename K, typename F, typename... Args>
  bool uprase_fn(K&& key, F fn, Args&&... val) {
    hash_value hv = hashed_key(key);
    table_position pos = linear_insert_loop(key, hv);
    assert(pos.status != failure);
    if (pos.status == ok) {
      // insert
      assert(pos.lock != nullptr);
      assert(!pos.lock->try_lock());
      add_to_bucket(pos.index, std::forward<K>(key),
                    std::forward<Args>(val)...);
    } else {
      // update or erase
      assert(pos.status == failure_key_duplicated);
      if (fn(buckets_[pos.index].mapped())) {
        del_from_bucket(pos.index);
      }
    }
    return pos.status == ok;
  }

  /**
   * @brief 删除的辅助函数
   *
   * @tparam K 待删除的键（Key）类型
   * @tparam F 对key所关联的value进行操作的类型
   * @param key 待删除的具体键（key）
   * @param fn 对key所关联的value所进行的操作
   * @return true key在哈希表中，操作完成
   * @return false key不在哈希表中
   */
  template <typename K, typename F>
  bool erase_fn(const K& key, F fn) {
    const hash_value hv = hashed_key(key);
    table_position pos = linear_find_loop(key, hv);
    if (pos.status == ok) {
      if (fn(buckets_[pos.index].mapped())) {
        del_from_bucket(pos.index);
      }
      return true;
    } else {
      return false;
    }
  }

  /**
   * @brief 插入操作的辅助函数
   *
   * @tparam K 待插入的键（Key）类型
   * @tparam Args 用于构造关联value的参数的类型
   * @param bucket_ind bucket索引值
   * @param key 待插入的具体键（key）
   * @param val 用于构造关联value的参数
   */
  template <typename K, typename... Args>
  void add_to_bucket(const size_type bucket_ind, K&& key, Args&&... val) {
    buckets_.setKV(bucket_ind, std::forward<K>(key),
                   std::forward<Args>(val)...);
    ++get_current_locks()[lock_ind(bucket_ind)].elem_counter();
  }

  /// 从内部存储（buckets）中删除指定索引处的键值对
  void del_from_bucket(const size_type bucket_ind) {
    buckets_.eraseKV(bucket_ind);
    --get_current_locks()[lock_ind(bucket_ind)].elem_counter();
  }

  /**
   * @brief 清空哈希表的辅助函数，不释放内存
   * @see clear_and_free()
   */
  void linear_clear() {
    buckets_.clear();
    for (spinlock_t& lock : get_current_locks()) {
      lock.elem_counter() = 0;
      lock.is_migrated() = true;
    }
  }

  /**
   * @brief 清空哈希表的辅助函数，释放内存
   * @see clear()
   */
  void linear_free() {
    buckets_.clear_and_deallocate();
    for (spinlock_t& lock : get_current_locks()) {
      lock.elem_counter() = 0;
      lock.is_migrated() = true;
    }
  }

  /// 并行执行辅助函数，用于哈希表扩容时均分迁移任务给多个线程执行
  template <typename F>
  void parallel_exec(size_type start, size_type end, F func) {
    const size_type num_extra_threads = max_num_worker_threads();
    const size_type num_workers = 1 + num_extra_threads;
    size_type work_per_thread = (end - start) / num_workers;
    std::vector<std::thread> threads;
    threads.reserve(num_extra_threads);

    std::vector<std::exception_ptr> eptrs(num_workers, nullptr);
    for (size_type i = 0; i < num_extra_threads; ++i) {
      threads.emplace_back(func, start, start + work_per_thread,
                           std::ref(eptrs[i]));
      start += work_per_thread;
    }
    func(start, end, std::ref(eptrs.back()));
    for (std::thread& t : threads) {
      t.join();
    }
    for (std::exception_ptr& eptr : eptrs) {
      if (eptr) std::rethrow_exception(eptr);
    }
  }

  /// rehash的辅助函数，非线程安全
  bool linear_rehash(size_type new_hp) {
    const size_type hp = hashpower();
    if (new_hp == hp) {
      return false;
    }
    return linear_expand(hp, new_hp) == ok;
  }

  /// reserve的辅助函数，非线程安全
  bool linear_reserve(size_type n) {
    const size_type hp = hashpower();
    const size_type new_hp = reserve_calc(n);
    if (new_hp == hp) {
      return false;
    }
    return linear_expand(hp, new_hp) == ok;
  }

  /// 工具函数，用于计算需要预留的内存空间
  static size_type reserve_calc(const size_type n) {
    const size_type buckets = n;
    size_type blog2;
    for (blog2 = 0; (size_type(1) << blog2) < buckets; ++blog2)
      ;
    assert(n <= buckets && buckets <= hashsize(blog2));
    return blog2;
  }

  template <typename K>
  hash_value hashed_key(const K& key) const {
    const size_type hash = hash_function()(key);
    return {hash};
  }

  static inline size_type hashsize(const size_type hp) {
    return size_type(1) << hp;
  }

  static inline size_type hashmask(const size_type hp) {
    return hashsize(hp) - 1;
  }

  static inline size_type index_hash(const size_type hp, const size_type hv) {
    return hv & hashmask(hp);
  }

  void maybe_resize_locks(size_type new_bucket_count, locks_t& new_locks) {
    locks_t next_locks(std::min(size_type(kMaxNumLocks), new_bucket_count));
    std::copy(new_locks.begin(), new_locks.end(), next_locks.begin());
    for (spinlock_t& lock : next_locks) {
      lock.lock();
    }
    all_locks_.emplace_back(std::move(next_locks));
  }

  op_status linear_expand(size_type orig_hp, size_type new_hp) {
    auto all_locks_manager = lock_all();
    if (!all_locks_manager) return failure;

    ++nr_expand_or_shrink;
    const size_type hp = hashpower();
    if (hp != orig_hp) {
      return failure_under_expansion;
    }
    map new_map(new_hp);
    new_map.max_num_worker_threads(max_num_worker_threads());
    parallel_exec(
        0, hashsize(hp),
        [this, &new_map](size_type i, size_type end, std::exception_ptr& eptr) {
          try {
            for (; i < end; ++i) {
              auto& bucket = buckets_[i];
              if (bucket.occupied() && !bucket.deleted()) {
                new_map.insert(bucket.movable_key(),
                               std::move(bucket.mapped()));
              }
            }
          } catch (...) {
            eptr = std::current_exception();
          }
        });
    maybe_resize_locks(new_map.bucket_count(), new_map.get_current_locks());
    buckets_.swap(new_map.buckets_);
    return ok;
  }

  /// 哈希函数
  hasher hash_fn_;
  /// 判别key是否相等的相等函数
  key_equal eq_fn_;
  /// 历史上和当前时刻所有自旋锁的集合
  mutable all_locks_t all_locks_;
  /// 当前使用的存储数据结构
  mutable buckets_t buckets_;
  /// 扩展前的存储数据结构
  mutable buckets_t old_buckets_;
  /// 保存扩容时可启动的线程数
  std::atomic<size_type> max_num_worker_threads_;

  /// 用于debug的统计数据，扩容或缩容次数
  uint64_t nr_expand_or_shrink = 0;
  /// 用于debug的统计数据，清空次数
  uint64_t nr_clear = 0;

 public:
  class locked_table {
   public:
    /// 导出rb_hashmap中定义的类型
    using buckets_t = typename map::buckets_t;
    using key_type = typename map::key_type;
    using mapped_type = typename map::mapped_type;
    using value_type = typename map::value_type;
    using size_type = typename map::size_type;
    using difference_type = typename map::difference_type;
    using hasher = typename map::hasher;
    using key_equal = typename map::key_equal;
    using reference = typename map::reference;
    using const_reference = typename map::const_reference;
    using pointer = typename map::pointer;
    using const_pointer = typename map::const_pointer;

    /// 按照实际存储顺序进行迭代的const迭代器
    class const_iterator {
     public:
      using difference_type = typename locked_table::difference_type;
      using value_type = typename locked_table::value_type;
      using pointer = typename locked_table::const_pointer;
      using reference = typename locked_table::const_reference;
      using iterator_category = std::bidirectional_iterator_tag;

      /// 默认构造函数
      const_iterator() = default;

      bool operator==(const const_iterator& it) const {
        return buckets_ == it.buckets_ && index_ == it.index_;
      }
      bool operator!=(const const_iterator& it) const {
        return !operator==(it);
      }
      /// 解引用操作符
      reference operator*() const { return (*buckets_)[index_].kvpair(); }
      /// 箭头操作符
      pointer operator->() const { return std::addressof(operator*()); }
      /// 前置++操作符
      const_iterator& operator++() {
        ++index_;
        for (; index_ < buckets_->size(); ++index_) {
          if ((*buckets_)[index_].occupied() &&
              !(*buckets_)[index_].deleted()) {
            return *this;
          }
        }
        assert(index_ == end_pos(*buckets_));
        return *this;
      }
      /// 后置++操作符
      const_iterator operator++(int) {
        const_iterator old(*this);
        ++(*this);
        return old;
      }
      /// 前置--操作符
      const_iterator& operator--() {
        --index_;
        if (index_ != 0) {
          while (!(*buckets_)[index_].occupied() ||
                 (*buckets_)[index_].deleted()) {
            if (--index_ == 0) {
              break;
            }
          }
        }
        return *this;
      }
      /// 后置--操作符
      const_iterator operator--(int) {
        const_iterator old(*this);
        --(*this);
        return old;
      }

     protected:
      buckets_t* buckets_ = nullptr;
      size_type index_ = 0;
      friend class locked_table;

      static size_type end_pos(const buckets_t& buckets) {
        return buckets.size();
      }

      /// 在指定bucket位置处构造迭代器，跳过可能的空或已经被删除的位置
      const_iterator(buckets_t& buckets, size_type index) noexcept
          : buckets_(std::addressof(buckets)), index_(index) {
        if (index_ != end_pos(*buckets_) && (!(*buckets_)[index_].occupied() ||
                                             (*buckets_)[index_].deleted())) {
          operator++();
        }
      }
    };

    /// 按照实际存储顺序进行迭代的迭代器
    class iterator : public const_iterator {
     public:
      using pointer = typename map::pointer;
      using reference = typename map::reference;

      iterator() = default;

      /// 相等操作符，使用基类中的实现
      bool operator==(const iterator& it) {
        return const_iterator::operator==(it);
      }
      /// 不等操作符，使用基类中的实现
      bool operator!=(const iterator& it) {
        return const_iterator::operator!=(it);
      }
      /// 解引用操作符
      reference operator*() {
        return (*const_iterator::buckets_)[const_iterator::index_].kvpair();
      }
      /// 箭头操作符
      pointer operator->() { return std::addressof(operator*()); }
      /// 前置++操作符，使用基类中的实现
      iterator& operator++() {
        const_iterator::operator++();
        return *this;
      }
      /// 后置++操作符，使用基类中的实现
      iterator operator++(int) {
        iterator old(*this);
        const_iterator::operator++();
        return old;
      }
      /// 前置--操作符，使用基类中的实现
      iterator& operator--() {
        const_iterator::operator--();
        return *this;
      }
      /// 后置--操作符，使用基类中的实现
      iterator operator--(int) {
        iterator old(*this);
        const_iterator::operator--();
        return old;
      }

     private:
      iterator(buckets_t& buckets, size_type index) noexcept
          : const_iterator(buckets, index) {}
      friend class locked_table;
    };

    /// 禁止显式的构造locked_table，必须通过rb_hashmap的接口lock_table()进行构造
    locked_table() = delete;
    /// 禁止locked_table对象的拷贝
    locked_table(const locked_table&) = delete;
    locked_table& operator=(const locked_table&) = delete;

    /// 移动构造函数
    locked_table(locked_table&& lt) noexcept
        : map_(std::move(lt.map_)),
          all_locks_manager_(std::move(lt.all_locks_manager_)) {}

    /// 移动赋值操作符
    locked_table& operator=(locked_table&& lt) noexcept {
      unlock();
      map_ = std::move(lt.map_);
      all_locks_manager_ = std::move(lt.all_locks_manager_);
      return *this;
    }

    /// 解锁
    void unlock() { all_locks_manager_.reset(); }

    /// 返回指向第一个元素的迭代器（按照存储顺序）
    iterator begin() { return iterator(map_.get().buckets_, 0); }

    /// 返回指向第一个元素的const迭代器（按照存储顺序）
    const_iterator begin() const {
      return const_iterator(map_.get().buckets_, 0);
    }

    /// 返回指向第一个元素的cosnt迭代器（按照存储顺序）
    const_iterator cbegin() const { return begin(); }

    /// 返回指向最后一个元素后面的迭代器（按照存储顺序）
    iterator end() {
      const auto eof = const_iterator::end_pos(map_.get().buckets_);
      return iterator(map_.get().buckets_, eof);
    }

    /// 返回指向最后一个元素后面的const迭代器（按照存储顺序）
    const_iterator end() const {
      const auto eof = const_iterator::end_pos(map_.get().buckets_);
      return const_iterator(map_.get().buckets_, eof);
    }
    /// 返回指向最后一个元素后面的const迭代器（按照存储顺序）
    const_iterator cend() const { return end(); }

   private:
    locked_table(map& map) noexcept
        : map_(map), all_locks_manager_(map.lock_all()) {}

    std::reference_wrapper<map> map_;
    AllLocksManager all_locks_manager_;
    friend class map;
  };
};

}  // namespace rbhash