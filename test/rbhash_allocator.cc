#include "rbhash/rbhash.hpp"
#include "rbhash_test.h"

#include <gtest/gtest.h>

std::atomic<int64_t>& get_unfreed_bytes()
{
    static std::atomic<int64_t> unfreed_bytes(0L);
    return unfreed_bytes;
}

TEST(Allocator, custom)
{
    IntIntTableWithCustomAllocator tbl;

    auto insertWorker = [&](int id) {
        for (int i = 0; i < 10000; ++i) {
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

    // <1KB
    EXPECT_LE(get_unfreed_bytes() - tbl.footprint(), 1000);
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}