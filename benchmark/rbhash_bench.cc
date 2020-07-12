#include "rbhash/rbhash.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

double rate(uint64_t counter, int64_t nanos)
{
    return counter * 1e3 * 1.0 / nanos;
}

enum Ops {
    READ,
    INSERT,
    ERASE,
    UPDATE,
    UPSERT,
};

void gen_nums(uint64_t start, std::vector<uint64_t>& nums,
    std::default_random_engine& rng)
{
    for (uint64_t& num : nums) {
        num = ++start;
    }
    std::shuffle(nums.begin(), nums.end(), rng);
}

void prefill(rbhash::map<uint64_t, uint64_t>& table,
    std::vector<uint64_t>& keys, uint64_t prefill_elems)
{
    for (size_t i = 0; i < prefill_elems; ++i) {
        if (!table.insert(keys[i], keys[i])) {
            abort();
        }
    }
}

void mix(rbhash::map<uint64_t, uint64_t>& table, uint64_t num_ops,
    std::array<Ops, 100> op_mix, std::vector<uint64_t>& nums,
    uint64_t prefill_elems)
{
    // numkeys is a power of 2
    uint64_t numkeys = nums.size(), v;
    assert((numkeys & (numkeys - 1)) == 0);
    size_t erase_seq = 0;
    size_t insert_seq = prefill_elems;

    auto upsert_fn = [](uint64_t& v) { return; };
    size_t n;
    size_t find_seq = 0;
    const size_t a = numkeys / 2 + 1;
    const size_t c = numkeys / 4 - 1;
    const size_t find_seq_mask = numkeys - 1;

    // update find index
    auto find_seq_update = [&]() {
        find_seq = (a * find_seq + c) & find_seq_mask;
    };

    for (int i = 0; i < num_ops;) {
        for (int j = 0; j < 100 && j < num_ops; ++j, ++i) {
            switch (op_mix[j]) {
            case READ: {
                bool r1 = find_seq >= erase_seq && find_seq < insert_seq;
                bool r2 = table.find(nums[find_seq], v);
                assert(r1 == r2);
                find_seq_update();
                break;
            }
            case INSERT: {
                bool ret = table.insert(nums[insert_seq], nums[insert_seq]);
                ++insert_seq;
                break;
            }
            case ERASE: {
                if (erase_seq == insert_seq) {
                    bool ret = table.erase(nums[find_seq]);
                    assert(!ret);
                    find_seq_update();
                } else {
                    bool ret = table.erase(nums[erase_seq++]);
                    assert(ret);
                }
                break;
            }
            case UPDATE: {
                bool r1 = find_seq >= erase_seq && find_seq < insert_seq;
                bool r2 = table.update(nums[find_seq], nums[find_seq]);
                assert(r1 == r2);
                find_seq_update();
                break;
            }
            case UPSERT: {
                n = std::min(find_seq, insert_seq);
                table.upsert(nums[n], upsert_fn, nums[n]);
                if (n == insert_seq) {
                    ++insert_seq;
                } else {
                    find_seq_update();
                }
                break;
            }
            default:
                assert(false);
            }
        }
    }
}

int main(int argc, char* argv[])
{
    uint64_t init_hashpower = 25;
    uint64_t read_percentage = 100;
    uint64_t insert_percentage = 0;
    uint64_t erase_percentage = 0;
    uint64_t update_percentage = 0;
    uint64_t upsert_percentage = 0;
    uint64_t perfill_percentage = 0;
    uint64_t total_ops_percentage = 70;
    uint64_t num_threads = std::thread::hardware_concurrency();
    uint64_t seed = std::random_device{}();

    for (int i = 1; i < argc; i++) {
        int n;
        char junk;
        if (sscanf(argv[i], "--init-size=%d%c", &n, &junk) == 1) {
            init_hashpower = n;
        } else if (sscanf(argv[i], "--reads=%d%c", &n, &junk) == 1) {
            read_percentage = n;
        } else if (sscanf(argv[i], "--inserts=%d%c", &n, &junk) == 1) {
            insert_percentage = n;
        } else if (sscanf(argv[i], "--erases=%d%c", &n, &junk) == 1) {
            erase_percentage = n;
        } else if (sscanf(argv[i], "--updates=%d%c", &n, &junk) == 1) {
            update_percentage = n;
        } else if (sscanf(argv[i], "--upserts=%d%c", &n, &junk) == 1) {
            upsert_percentage = n;
        } else if (sscanf(argv[i], "--prefill=%d%c", &n, &junk) == 1) {
            perfill_percentage = n;
        } else if (sscanf(argv[i], "--total-ops=%d%c", &n, &junk) == 1) {
            total_ops_percentage = n;
        } else if (sscanf(argv[i], "--num-threads=%d%c", &n, &junk) == 1) {
            num_threads = n;
        } else if (sscanf(argv[i], "--seed=%d%c", &n, &junk) == 1) {
            seed = n;
        } else {
            std::fprintf(stderr, "Invalid flag '%s'\n", argv[i]);
            std::exit(1);
        }
    }

    if (read_percentage + insert_percentage + erase_percentage + update_percentage + upsert_percentage != 100) {
        std::cout << "The sum of read, insert, erase, update, and upsert "
                     "percentages must be 100"
                  << std::endl;
        std::exit(1);
    }

    const size_t initial_capacity = 1UL << init_hashpower;
    const size_t total_ops = initial_capacity * total_ops_percentage / 100;

    rbhash::map<uint64_t, uint64_t> tbl(init_hashpower);
    std::default_random_engine di(seed);

    std::array<Ops, 100> op_mix;
    auto* op_mix_p = &op_mix[0];
    for (size_t i = 0; i < read_percentage; ++i) {
        *op_mix_p++ = READ;
    }
    for (size_t i = 0; i < insert_percentage; ++i) {
        *op_mix_p++ = INSERT;
    }
    for (size_t i = 0; i < erase_percentage; ++i) {
        *op_mix_p++ = ERASE;
    }
    for (size_t i = 0; i < update_percentage; ++i) {
        *op_mix_p++ = UPDATE;
    }
    for (size_t i = 0; i < upsert_percentage; ++i) {
        *op_mix_p++ = UPSERT;
    }
    std::shuffle(op_mix.begin(), op_mix.end(), di);

    const size_t prefill_elems = initial_capacity * perfill_percentage / 100;
    const size_t max_insert_ops = (total_ops + 99) / 100 * (insert_percentage + erase_percentage);

    const size_t insert_keys = std::max(initial_capacity, max_insert_ops) + prefill_elems;

    const size_t insert_keys_per_thread = static_cast<size_t>(ceil((insert_keys + num_threads - 1) / num_threads));
    std::vector<std::vector<uint64_t>> nums(num_threads);

    for (size_t i = 0; i < num_threads; ++i) {
        nums[i].resize(insert_keys_per_thread);
        gen_nums(i * insert_keys_per_thread, nums[i], di);
    }

    std::cout << "Generate test data done\n";
    std::vector<std::thread> prefill_threads(num_threads);
    const size_t prefill_elems_per_thread = prefill_elems / num_threads;
    assert(insert_keys_per_thread > prefill_elems_per_thread);

    for (size_t i = 0; i < num_threads; ++i) {
        prefill_threads[i] = std::thread(prefill, std::ref(tbl), std::ref(nums[i]),
            prefill_elems_per_thread);
    }
    for (auto& t : prefill_threads) {
        t.join();
    }

    std::vector<std::thread> mix_threads(num_threads);
    const size_t num_ops_per_thread = total_ops / num_threads;
    std::cout << "Start execuating: table size: " << tbl.size()
              << ", table capacity: " << tbl.capacity() << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < num_threads; ++i) {
        mix_threads[i] = std::thread(mix, std::ref(tbl), num_ops_per_thread, std::ref(op_mix),
            std::ref(nums[i]), prefill_elems_per_thread);
    }
    for (auto& t : mix_threads) {
        t.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double seconds_elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time)
                                 .count();

    std::cout << "init-size: " << init_hashpower << ", "
              << "prefill: " << perfill_percentage << ", "
              << "total-ops: " << total_ops << ", "
              << "read: " << read_percentage << "%, "
              << "insert: " << insert_percentage << "%, "
              << "erase: " << erase_percentage << "%, "
              << "update: " << update_percentage << "%, "
              << "upsert: " << upsert_percentage << "%\n"
              << "End mixing: total ops: " << total_ops << ", seed: " << seed
              << ", num_threads: " << num_threads
              << ", elapse: " << seconds_elapsed
              << " s, throughput: " << total_ops / seconds_elapsed
              << ", average latency/op: "
              << ((end_time - start_time).count() / total_ops) << " ns\n";
    std::cout << tbl.stat() << std::endl;
    return 0;
}