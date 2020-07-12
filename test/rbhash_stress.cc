#include "rbhash/rbhash.hpp"
#include "rbhash_test.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <vector>

typedef uint32_t KeyType;
typedef std::string KeyType2;
typedef uint32_t ValueType;
typedef int32_t ValueType2;

constexpr int g_keypower = 24;
constexpr int g_numkeys = 1UL << g_keypower;
constexpr int g_thread_num = 4;
constexpr int g_test_len = 10;
uint64_t g_seed = 0;

namespace stress {

std::atomic<size_t> num_inserts = ATOMIC_VAR_INIT(0);
std::atomic<size_t> num_deletes = ATOMIC_VAR_INIT(0);
std::atomic<size_t> num_updates = ATOMIC_VAR_INIT(0);
std::atomic<size_t> num_finds = ATOMIC_VAR_INIT(0);

template <typename Key>
class AllEnvironment {
public:
    AllEnvironment()
        : table(g_keypower)
        , table2(g_keypower)
        , keys(g_numkeys)
        , vals(g_numkeys)
        , vals2(g_numkeys)
        , in_table(new bool[g_numkeys])
        , in_use(new std::atomic_flag[g_numkeys])
        , val_dist(std::numeric_limits<ValueType>::min(),
              std::numeric_limits<ValueType>::max())
        , val_dist2(std::numeric_limits<ValueType2>::min(),
              std::numeric_limits<ValueType2>::max())
        , ind_dist(0, g_numkeys - 1)
        , finished(false)
    {
        if (g_seed == 0) {
            g_seed = std::chrono::system_clock::now().time_since_epoch().count();
            // g_seed = 1592635802765785600LL;
        }

        // std::cout << "seed = " << g_seed << std::endl;
        gen_seed = g_seed;
        for (size_t i = 0; i < g_numkeys; ++i) {
            keys[i] = generateKey<Key>(i);
            in_table[i] = false;
            in_use[i].clear();
        }
    }

public:
    rbhash::map<Key, ValueType> table;
    rbhash::map<Key, ValueType2> table2;

    std::vector<Key> keys;
    std::vector<ValueType> vals;
    std::vector<ValueType2> vals2;

    std::unique_ptr<bool[]> in_table;
    std::unique_ptr<std::atomic_flag[]> in_use;

    std::uniform_int_distribution<ValueType> val_dist;
    std::uniform_int_distribution<ValueType2> val_dist2;

    std::uniform_int_distribution<size_t> ind_dist;

    size_t gen_seed;

    // when set true, it signals the threads to stop
    std::atomic<bool> finished;
};

template <typename Key>
void insert_thread(AllEnvironment<Key>* env)
{
    std::mt19937_64 gen(env->gen_seed);
    while (!env->finished.load()) {
        size_t ind = env->ind_dist(gen);
        if (!env->in_use[ind].test_and_set()) {
            Key k = env->keys[ind];
            ValueType v = env->val_dist(gen);
            ValueType2 v2 = env->val_dist2(gen);

            bool res = env->table.insert(k, v);
            bool res2 = env->table2.insert(k, v2);

            ASSERT_NE(res, env->in_table[ind]) << "index: " << ind << ", k: " << k;
            ASSERT_NE(res2, env->in_table[ind]) << "index: " << ind << ", k: " << k;

            if (res) {
                ASSERT_EQ(v, env->table.find(k));
                ASSERT_EQ(v2, env->table2.find(k));

                env->vals[ind] = v;
                env->vals2[ind] = v2;

                env->in_table[ind] = true;
                num_inserts.fetch_add(2, std::memory_order_relaxed);
            }
            env->in_use[ind].clear();
        }
    }
}

template <typename Key>
void update_thread(AllEnvironment<Key>* env)
{
    std::mt19937_64 gen(env->gen_seed);
    std::uniform_int_distribution<size_t> third(0, 2);

    auto updatefn = [](ValueType& v) { v += 3; };
    auto updatefn2 = [](ValueType2& v) { v += 10; };

    while (!env->finished.load()) {
        size_t ind = env->ind_dist(gen);
        if (!env->in_use[ind].test_and_set()) {
            Key k = env->keys[ind];
            ValueType v;
            ValueType2 v2;

            bool res, res2;
            switch (third(gen)) {
            case 0:
                // update
                v = env->val_dist(gen);
                v2 = env->val_dist2(gen);

                res = env->table.update(k, v);
                res2 = env->table2.update(k, v2);

                ASSERT_EQ(res, env->in_table[ind]);
                ASSERT_EQ(res2, env->in_table[ind]);
                break;
            case 1:
                // update_fn
                v = env->vals[ind];
                v2 = env->vals2[ind];

                updatefn(v);
                updatefn2(v2);

                res = env->table.update_fn(k, updatefn);
                res2 = env->table2.update_fn(k, updatefn2);

                ASSERT_EQ(res, env->in_table[ind]);
                ASSERT_EQ(res2, env->in_table[ind]);
                break;
            case 2:
                // upsert
                if (env->in_table[ind]) {
                    v = env->vals[ind];
                    v2 = env->vals2[ind];
                    updatefn(v);
                    updatefn2(v2);
                } else {
                    v = env->val_dist(gen);
                    v2 = env->val_dist2(gen);
                }

                env->table.upsert(k, updatefn, v);
                env->table2.upsert(k, updatefn2, v2);
                res = res2 = true;
                env->in_table[ind] = true;
                break;
            default:
                throw std::logic_error("impossible");
            }

            if (res) {
                ASSERT_EQ(v, env->table.find(k)) << "Key: " << k << ", v: " << v;
                ASSERT_EQ(v2, env->table2.find(k)) << "Key: " << k << ", v: " << v;
                env->vals[ind] = v;
                env->vals2[ind] = v2;
                num_updates.fetch_add(2, std::memory_order_relaxed);
            }
            env->in_use[ind].clear();
        }
    }
}

template <typename Key>
void find_thread(AllEnvironment<Key>* env)
{
    std::mt19937_64 gen(env->gen_seed);
    while (!env->finished.load()) {
        size_t ind = env->ind_dist(gen);
        if (!env->in_use[ind].test_and_set()) {
            Key k = env->keys[ind];
            try {
                ASSERT_EQ(env->vals[ind], env->table.find(k));
                ASSERT_TRUE(env->in_table[ind]);
            } catch (const std::out_of_range&) {
                ASSERT_FALSE(env->in_table[ind]);
            }

            try {
                ASSERT_EQ(env->vals2[ind], env->table2.find(k));
                ASSERT_TRUE(env->in_table[ind]);
            } catch (const std::out_of_range&) {
                ASSERT_FALSE(env->in_table[ind]);
            }

            num_finds.fetch_add(2, std::memory_order_relaxed);
            env->in_use[ind].clear();
        }
    }
}

template <typename Key>
void StressTest(AllEnvironment<Key>* env)
{
    std::vector<std::thread> threads;
    for (size_t i = 0; i < g_thread_num; ++i) {
        threads.emplace_back(insert_thread<Key>, env);
        // TODO: delete thread
        threads.emplace_back(update_thread<Key>, env);
        threads.emplace_back(find_thread<Key>, env);
    }

    std::this_thread::sleep_for(std::chrono::seconds(g_test_len));
    env->finished.store(true);
    for (auto& t : threads) {
        t.join();
    }

    size_t numfilled = 0;
    for (size_t i = 0; i < g_numkeys; ++i) {
        if (env->in_table[i]) {
            numfilled++;
        }
    }
    ASSERT_EQ(numfilled, env->table.size());
}

} // namespace stress

TEST(Stress, CheckedIntKey)
{
    stress::num_inserts = 0;
    stress::num_deletes = 0;
    stress::num_updates = 0;
    stress::num_finds = 0;

    stress::AllEnvironment<KeyType> env;
    stress::StressTest(&env);
}

TEST(Stress, CheckedStringKey)
{
    stress::num_inserts = 0;
    stress::num_deletes = 0;
    stress::num_updates = 0;
    stress::num_finds = 0;

    stress::AllEnvironment<KeyType2> env;
    stress::StressTest(&env);
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}