#include "rbhash/rbhash.hpp"
#include "rbhash_test.h"

#include <gtest/gtest.h>

TEST(Iterate, Basic)
{
    IntIntTable tbl(1);
    {
        auto it = tbl.lock_table();
        EXPECT_TRUE(it.begin() == it.begin());
        EXPECT_TRUE(it.begin() == it.end());
        EXPECT_TRUE(it.end() == it.end());
        EXPECT_TRUE(it.cbegin() == it.cbegin());
        EXPECT_TRUE(it.cbegin() == it.begin());
        EXPECT_TRUE(it.cbegin() == it.cend());
        EXPECT_TRUE(it.cend() == it.cend());
    }

    tbl.insert(0, 0);
    tbl.insert(1, 1);
    tbl.insert(2, 2);
    tbl.insert(3, 3);
    int d;
    for (int i = 0; i < 4; ++i) {
        EXPECT_TRUE(tbl.find(i, d));
    }

    {
        auto locked = tbl.lock_table();
        auto it = locked.begin();
        EXPECT_EQ(it->first, 0);
        ++it;
        EXPECT_EQ(it->first, 1);
        ++it;
        EXPECT_EQ(it->first, 2);
        ++it;
        EXPECT_EQ(it->first, 3);
        --it;
        EXPECT_EQ(it->first, 2);
        it--;
        EXPECT_EQ(it->first, 1);
        --it;
        EXPECT_EQ(it->first, 0);
    }

    {
        auto locked = tbl.lock_table();
        auto it = locked.cbegin();
        EXPECT_EQ(it->first, 0);
        auto it2 = it++;
        EXPECT_EQ(it2->first, 0);
        EXPECT_EQ((*it).first, 1);
        auto it3 = it--;
        EXPECT_EQ(it3->first, 1);
        EXPECT_EQ(it->first, 0);
    }

    tbl.clear();
    std::map<int, int> comp;
    EXPECT_TRUE(tbl.empty());
    constexpr int kSize = 1024;
    {
        for (int i = 0; i < kSize; ++i) {
            EXPECT_TRUE(tbl.insert(i, i));
            comp[i] = i;
        }

        int value, counter = 0;
        for (const auto& p : comp) {
            ++counter;
            EXPECT_TRUE(tbl.find(p.first, value));
            EXPECT_EQ(value, p.second);
        }
        EXPECT_EQ(counter, kSize);

        EXPECT_EQ(tbl.size(), kSize);
        EXPECT_GE(tbl.hashpower(), 10);
        EXPECT_EQ(tbl.capacity(), tbl.size());

        auto locked = tbl.lock_table();
        auto it = locked.begin();
        auto it2 = locked.end();

        EXPECT_TRUE(it != it2);
        counter = 0;
        for (auto& p : locked) {
            ++counter;
            EXPECT_TRUE(comp.find(p.first) != comp.end());
            EXPECT_EQ(p.second, comp[p.first]);
        }
        EXPECT_EQ(counter, kSize);
    }
}

TEST(Iterate, Moderate)
{
    IntIntTable tbl(10);
    int counter = 1000000;
    for (int i = 0; i < counter; ++i) {
        EXPECT_TRUE(tbl.insert(i, i));
    }

    // delete odd numbers
    for (int i = 1; i < counter; i += 2) {
        EXPECT_TRUE(tbl.erase(i));
    }

    {
        auto locked = tbl.lock_table();
        int i = 0;
        for (auto it = locked.begin(); it != locked.end(); ++it) {
            EXPECT_EQ(it->first, i);
            EXPECT_EQ(it->second, i);
            i += 2;
        }
    }

    // deleate all numbers which is not power of 2
    EXPECT_TRUE(tbl.erase(0));
    for (int i = 2; i < counter; i += 2) {
        if ((i & (i - 1)) != 0) {
            EXPECT_TRUE(tbl.erase(i)) << i;
        }
    }

    {
        auto locked = tbl.lock_table();
        int check = 2;
        for (auto it = locked.begin(); it != locked.end(); ++it) {
            EXPECT_EQ(it->first, check);
            EXPECT_EQ(it->second, check);
            check *= 2;
        }
    }
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}