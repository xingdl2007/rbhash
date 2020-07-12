#include "rbhash/rbhash.hpp"
#include "rbhash_test.h"

#include <gtest/gtest.h>

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <thread>
#include <unistd.h>

struct Data {
};

TEST(Operation, Insert)
{
    rbhash::map<int, Data> tbl(1);
    EXPECT_EQ(tbl.capacity(), 2);

    tbl.insert(1, Data());
    tbl.insert(2, Data());
    tbl.insert(3, Data());

    EXPECT_EQ(tbl.capacity(), 4);

    tbl.insert(4, Data());
    tbl.insert(5, Data());

    EXPECT_EQ(tbl.capacity(), 8);
    tbl.insert_or_assign(6, Data());
    tbl.insert_or_assign(7, Data());
    tbl.insert_or_assign(8, Data());
    tbl.insert_or_assign(9, Data());
    EXPECT_EQ(tbl.capacity(), 16);

    Data d;
    EXPECT_TRUE(tbl.find(1, d));
    EXPECT_TRUE(tbl.find(2, d));
    EXPECT_TRUE(tbl.find(3, d));
    EXPECT_TRUE(tbl.find(4, d));
    EXPECT_TRUE(tbl.find(5, d));
    EXPECT_TRUE(tbl.find(6, d));
    EXPECT_TRUE(tbl.find(7, d));
    EXPECT_TRUE(tbl.find(8, d));
    EXPECT_TRUE(tbl.find(9, d));

    EXPECT_FALSE(tbl.find(10, d));
    EXPECT_FALSE(tbl.find(11, d));
    EXPECT_FALSE(tbl.find(12, d));
    EXPECT_FALSE(tbl.find(13, d));
    EXPECT_FALSE(tbl.find(14, d));
    EXPECT_FALSE(tbl.find(15, d));
    EXPECT_FALSE(tbl.find(16, d));

    EXPECT_EQ(tbl.capacity(), 16);
}

TEST(Operation, InsertPressure)
{
    rbhash::map<uint64_t, uint64_t> tbl;
    for (uint64_t i = 0; i < 1000000; ++i) {
        EXPECT_TRUE(tbl.insert(i, i));
    }

    uint64_t value = 0;
    for (uint64_t i = 0; i < 1000000; ++i) {
        EXPECT_TRUE(tbl.find(i, value));
        EXPECT_EQ(i, value);
    }
}

TEST(Operation, Clear)
{
    rbhash::map<uint64_t, uint64_t> tbl;
    for (uint64_t i = 0; i < 100; ++i) {
        EXPECT_TRUE(tbl.insert(i, i));
    }

    uint64_t value = 0;
    for (uint64_t i = 0; i < 100; ++i) {
        EXPECT_TRUE(tbl.find(i, value));
        EXPECT_EQ(i, value);
    }

    tbl.clear();
    EXPECT_EQ(tbl.size(), 0);
    value = 0;
    for (uint64_t i = 0; i < 100; ++i) {
        EXPECT_FALSE(tbl.find(i, value));
        EXPECT_EQ(value, 0);
    }
}

TEST(Operation, Find)
{
    rbhash::map<int, Data> tbl(10);
    for (int i = 0; i < tbl.capacity(); ++i) {
        EXPECT_TRUE(tbl.insert(i, Data{}));
    }
    Data d;
    for (int i = 0; i < tbl.capacity(); ++i) {
        EXPECT_TRUE(tbl.find(i, d));
    }
    for (int i = tbl.capacity(); i < 2 * tbl.capacity(); ++i) {
        EXPECT_FALSE(tbl.find(i, d));
    }
}

TEST(Operation, Delete)
{
    rbhash::map<int, Data> tbl(1);
    EXPECT_EQ(tbl.capacity(), 2);

    tbl.insert(1, Data{});
    tbl.insert(2, Data{});
    tbl.insert(3, Data{});
    tbl.insert(4, Data{});

    Data d;
    EXPECT_TRUE(tbl.find(1, d));
    EXPECT_TRUE(tbl.find(2, d));
    EXPECT_TRUE(tbl.find(3, d));
    EXPECT_TRUE(tbl.find(4, d));

    EXPECT_TRUE(tbl.erase(1));
    EXPECT_TRUE(tbl.erase(2));
    EXPECT_TRUE(tbl.erase(3));
    EXPECT_TRUE(tbl.erase(4));

    EXPECT_FALSE(tbl.erase(1));
    EXPECT_FALSE(tbl.erase(2));
    EXPECT_FALSE(tbl.erase(3));
    EXPECT_FALSE(tbl.erase(4));

    EXPECT_FALSE(tbl.find(1, d));
    EXPECT_FALSE(tbl.find(2, d));
    EXPECT_FALSE(tbl.find(3, d));
    EXPECT_FALSE(tbl.find(4, d));

    EXPECT_EQ(tbl.capacity(), 4);
    EXPECT_EQ(tbl.load_factor(), 0);
}

TEST(Operation, Extent)
{
    constexpr int64_t size = 1 << 12;
    rbhash::map<int, Data> tbl(1);
    for (int i = 0; i < size; ++i) {
        EXPECT_TRUE(tbl.insert(i, Data()));
    }
    EXPECT_EQ(tbl.load_factor(), 1);
    Data d;
    for (int i = 0; i < size; ++i) {
        EXPECT_TRUE(tbl.find(i, d));
    }
    for (int i = 0; i < size; ++i) {
        EXPECT_TRUE(tbl.erase(i));
    }
    for (int i = 0; i < size; ++i) {
        EXPECT_FALSE(tbl.find(i, d));
    }
    EXPECT_EQ(tbl.capacity(), size);
    EXPECT_EQ(tbl.load_factor(), 0);
}

TEST(Operation, Shrink)
{
    constexpr int64_t size = 1 << 12;
    rbhash::map<int, Data> tbl(1);
    for (int i = 0; i < size; ++i) {
        EXPECT_TRUE(tbl.insert(i, Data()));
    }
    EXPECT_EQ(tbl.load_factor(), 1);
    for (int i = 0; i < size; ++i) {
        EXPECT_TRUE(tbl.erase(i));
    }
    tbl.shrink();
    EXPECT_EQ(tbl.load_factor(), 0);
    EXPECT_EQ(tbl.capacity(), 2);
}

TEST(Operation, Rehash)
{
    IntIntTable tbl(0);
    tbl.rehash(10);
    tbl.rehash(10);
    EXPECT_EQ(tbl.capacity(), 1 << 10);
}

TEST(Operation, Reserve)
{
    IntIntTable tbl(10);
    tbl.reserve(10);
    tbl.reserve(10);
    EXPECT_EQ(tbl.capacity(), 16);
}

TEST(MultiThreading, InsertFind)
{
    rbhash::map<uint64_t, uint64_t> tbl(1);
    EXPECT_EQ(tbl.hashpower(), 1);

    constexpr uint64_t counter = 1 << 10;

    auto insertWorker = [&](uint64_t id) {
        for (uint64_t i = 0; i < counter; ++i) {
            int cond = id & 3;
            if (cond == 0) {
                EXPECT_TRUE(tbl.insert(4 * i, 4 * i)) << 4 * i;
            } else if (cond == 1) {
                EXPECT_TRUE(tbl.insert(4 * i + 1, 4 * i + 1)) << 4 * i + 1;
            } else if (cond == 2) {
                EXPECT_TRUE(tbl.insert(4 * i + 2, 4 * i + 2)) << 4 * i + 2;
            } else if (cond == 3) {
                EXPECT_TRUE(tbl.insert(4 * i + 3, 4 * i + 3)) << 4 * i + 3;
            }
        }
    };

    std::thread t0(insertWorker, 0);
    std::thread t1(insertWorker, 1);
    std::thread t2(insertWorker, 2);
    std::thread t3(insertWorker, 3);

    t0.join();
    t1.join();
    t2.join();
    t3.join();

    // checker
    EXPECT_GE(tbl.capacity(), 4 * counter);
    uint64_t d;
    for (uint64_t i = 0; i < 4 * counter; ++i) {
        EXPECT_TRUE(tbl.find(i, d)) << i;
        EXPECT_EQ(i, d);
    }
}

TEST(Multithreading, InsertFindDelete)
{
    // start from small size
    rbhash::map<uint64_t, Data> tbl(1);
    constexpr uint64_t counter = 32;

    auto insertWorker = [&](uint64_t id) {
        for (uint64_t i = 0; i < counter; ++i) {
            if ((id & 1) == 0) {
                EXPECT_TRUE(tbl.insert(2 * i, Data())) << 2 * i; // even
            } else {
                EXPECT_TRUE(tbl.insert(2 * i + 1, Data())) << 2 * i + 1; // odd
            }
        }
    };

    auto deletorWorker = [&]() {
        for (uint64_t i = 0; i < 2 * counter; ++i) {
            while ((i % 3) == 0) {
                if (tbl.erase(i))
                    break;
                usleep(1000);
            }
        }
    };

    std::thread t0(insertWorker, 0);
    std::thread t1(insertWorker, 1);
    std::thread t2(deletorWorker);

    t0.join();
    t1.join();
    t2.join();

    // checker
    EXPECT_EQ(tbl.capacity(), 2 * counter);
    Data d;
    for (uint64_t i = 0; i < 2 * counter; ++i) {
        if ((i % 3) == 0) {
            EXPECT_FALSE(tbl.find(i, d)) << i;
        } else {
            EXPECT_TRUE(tbl.find(i, d)) << i;
        }
    }
}

TEST(StringTable, Basic)
{
    constexpr int size = 1 << 14;
    StringIntTable tbl;
    for (int i = 0; i < size; ++i) {
        EXPECT_TRUE(tbl.insert(generateKey<std::string>(i), i));
    }
    for (int i = 0; i < size; ++i) {
        EXPECT_EQ(i, tbl.find(generateKey<std::string>(i)));
    }
    for (int i = 0; i < size; ++i) {
        EXPECT_TRUE(tbl.erase(generateKey<std::string>(i)));
    }
    int value;
    for (int i = 0; i < size; ++i) {
        EXPECT_FALSE(tbl.find(generateKey<std::string>(i), value));
    }
}

TEST(StringTable, InsertDeleteFind)
{
    // start from small size
    StringIntTable tbl(1);
    constexpr uint64_t counter = 4;

    auto insertWorker = [&](uint64_t id) {
        for (uint64_t i = 0; i < counter; ++i) {
            if ((id & 1) == 0) {
                EXPECT_TRUE(tbl.insert(generateKey<std::string>(2 * i), 2 * i));
            } else {
                EXPECT_TRUE(tbl.insert(generateKey<std::string>(2 * i + 1), 2 * i + 1));
            }
        }
    };

    auto deletorWorker = [&]() {
        for (uint64_t i = 0; i < 2 * counter; ++i) {
            while ((i % 3) == 0) {
                if (tbl.erase(generateKey<std::string>(i)))
                    break;
                usleep(1000);
            }
        }
    };

    std::thread t0(insertWorker, 0);
    std::thread t1(insertWorker, 1);
    std::thread t2(deletorWorker);

    t0.join();
    t1.join();
    t2.join();

    int d;
    for (uint64_t i = 0; i < 2 * counter; ++i) {
        if ((i % 3) == 0) {
            EXPECT_FALSE(tbl.find(generateKey<std::string>(i), d)) << i;
        } else {
            EXPECT_TRUE(tbl.find(generateKey<std::string>(i), d)) << i;
        }
    }

    EXPECT_GE(tbl.capacity(), 2 * counter);
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}