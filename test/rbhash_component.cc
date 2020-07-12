#include "rbhash/rbhash.hpp"
#include "rbhash_test.h"

#include <gtest/gtest.h>

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <thread>
#include <unistd.h>

std::atomic<uint64_t> dummy::live{ 0 };
std::atomic<uint64_t> dummy::deleted{ 0 };

void reset()
{
    dummy::live = 0;
    dummy::deleted = 0;
}

// 对哈希表组件Table进行基本测试
TEST(Components, table)
{
    reset();
    constexpr size_t hashpower = 10;
    rbhash::table<int, dummy, std::allocator<int>> table(hashpower);
    EXPECT_EQ(table.hashpower(), hashpower);
    EXPECT_EQ(table.size(), 1 << hashpower);

    for (size_t i = 0; i < table.size(); ++i) {
        table.setKV(i, i, dummy()); // index, key, value
        EXPECT_TRUE(table[i].occupied());
        EXPECT_FALSE(table[i].deleted());
    }

    // default + move constructed
    EXPECT_EQ(dummy::live.load(std::memory_order_relaxed), 2 * table.size());
    table.clear();

    // all lived dummy object destructed finally
    // 由于存储的是dummy类型的对象，确保队列元素的正常析构
    EXPECT_EQ(dummy::live.load(std::memory_order_relaxed),
        dummy::deleted.load(std::memory_order_relaxed));
    table.destroy_buckets();
}

// 测试哈希表组件Table的构造
TEST(Components, TableConstruct)
{
    constexpr size_t hashpower = 10;
    rbhash::table<int, dummy, std::allocator<int>> table(hashpower);
    EXPECT_EQ(table.hashpower(), hashpower);
    EXPECT_EQ(table.size(), 1 << hashpower);

    for (size_t i = 0; i < table.size(); ++i) {
        table.setKV(i, i, dummy()); // index, key, value
        EXPECT_TRUE(table[i].occupied());
        EXPECT_FALSE(table[i].deleted());
    }

    // 测试移动构造
    rbhash::table<int, dummy, std::allocator<int>> table2(std::move(table));
    table.clear();
    table.destroy_buckets();

    rbhash::table<int, dummy, std::allocator<int>> table3;
    table3 = std::move(table2);
    table2.clear_and_deallocate();
    table3.clear_and_deallocate();
}

// 测试哈希表中spinlock组件
TEST(Components, spinlock)
{
    rbhash::spinlock_t lock;
    constexpr int time = 100;
    int counter = 0;
    auto increment = [&]() {
        for (int i = 0; i < time; ++i) {
            lock.lock();
            ++counter;
            lock.unlock();
        }
    };

    std::thread t0(increment);
    std::thread t1(increment);
    std::thread t2(increment);
    std::thread t3(increment);

    t0.join();
    t1.join();
    t2.join();
    t3.join();

    EXPECT_EQ(counter, 4 * time);

    auto lock2(lock);
    EXPECT_EQ(lock2.elem_counter(), lock.elem_counter());
    EXPECT_EQ(lock2.is_migrated(), lock.is_migrated());
    EXPECT_TRUE(lock2.try_lock());
    lock2.unlock();
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}