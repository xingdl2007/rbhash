#include "rbhash/rbhash.hpp"
#include "rbhash_test.h"

#include <gtest/gtest.h>

// 默认构造
TEST(Construct, DefaultSize)
{
    IntIntTable tbl;

    // 检查默认构造情况下的约束
    EXPECT_EQ(tbl.size(), 0);
    EXPECT_TRUE(tbl.empty());
    EXPECT_EQ(tbl.hashpower(), HASHMAP_DEFAULT_HASHPOWER);
    EXPECT_EQ(tbl.bucket_count(), 1UL << tbl.hashpower());
    EXPECT_EQ(tbl.load_factor(), 0);
}

TEST(Construct, GivenSize)
{
    IntIntTable tbl(1);
    EXPECT_EQ(tbl.size(), 0);
    EXPECT_TRUE(tbl.empty());
    EXPECT_EQ(tbl.hashpower(), 1);
    EXPECT_EQ(tbl.bucket_count(), tbl.capacity());
    EXPECT_EQ(tbl.capacity(), 1UL << tbl.hashpower());
    EXPECT_EQ(tbl.load_factor(), 0);
}

TEST(Construct, InitialList)
{
    IntIntTable tbl({ { 1, 2 }, { 3, 4 }, { 5, 6 }, { 7, 8 } });
    EXPECT_EQ(tbl.size(), 4);
    for (int i = 1; i <= 7; i += 2) {
        EXPECT_TRUE(tbl.find(i) == (i + 1));
    }
}

TEST(Construct, Move)
{
    IntIntTable tbl({ { 1, 2 }, { 3, 4 }, { 5, 6 }, { 7, 8 } });
    auto tbl2 = std::move(tbl);

    EXPECT_EQ(tbl.size(), 0);
    EXPECT_TRUE(tbl.empty());
    EXPECT_EQ(tbl2.size(), 4);

    int value = 0;
    for (int i = 1; i <= 7; i += 2) {
        EXPECT_TRUE(tbl2.find(i) == (i + 1));
    }
}

TEST(Construct, Zero)
{
    IntIntTable tbl(0);
    for (int i = 0; i < 10; ++i) {
        tbl.insert(i, i);
    }
    SUCCEED();
}

TEST(Stat, Size1)
{
    IntIntTable tbl(0);
    for (int i = 0; i < 1024; ++i) {
        tbl.insert(i, i);
        ASSERT_EQ(tbl.size(), i + 1);
    }
}

TEST(Stat, Size2)
{
    constexpr int counter = 10240;
    IntIntTable tbl(0);

    auto insertWorker = [&](uint64_t id) {
        for (int i = 0; i < counter; ++i) {
            if ((id & 1) == 0) {
                EXPECT_TRUE(tbl.insert(2 * i, 2 * i)) << 2 * i;
            } else {
                EXPECT_TRUE(tbl.insert(2 * i + 1, 2 * i + 1)) << 2 * i + 1;
            }
        }
    };

    std::thread t0(insertWorker, 0);
    std::thread t1(insertWorker, 1);

    t0.join();
    t1.join();
    EXPECT_EQ(tbl.size(), counter * 2);
}

TEST(Stat, Size3)
{
    IntIntTable tbl(0);
    int num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> prefill_threads(num_threads);
    const size_t insert_keys_per_thread = 10240;

    for (size_t i = 0; i < num_threads; ++i) {
        prefill_threads[i] = std::thread(
            [&](int id) {
                int start = insert_keys_per_thread * id;
                for (int i = start; i < insert_keys_per_thread + start; ++i) {
                    EXPECT_TRUE(tbl.insert(i, i));
                }
            },
            i);
    }

    for (auto& t : prefill_threads) {
        t.join();
    }
    EXPECT_EQ(tbl.size(), insert_keys_per_thread * num_threads);
}

TEST(Stat, Load)
{
    int hp = 20;
    IntIntTable tbl(hp);
    for (int i = 0; i < (1 << hp); ++i) {
        EXPECT_TRUE(tbl.insert(i, i));
    }

    EXPECT_EQ(tbl.size(), (1 << hp));
    EXPECT_EQ(tbl.capacity(), (1 << hp));
    EXPECT_EQ(tbl.load_factor(), 1.0);
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}