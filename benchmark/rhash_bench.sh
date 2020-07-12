#!/bin/bash

threads=(1 2 3 4 5 6 7 8)

# scalability mix: 70% read, 10% insert, 10% erase, 5% update, 5% upserts
printf "mix:\n"
for i in "${threads[@]}"; do
    ../build/benchmark/rhash_bench --prefill=50 --reads=70 --inserts=10 --erases=10 --updates=5 --upserts=5 --num-threads="$i"
done

# run once
printf "\nrun once:\n"
../build/benchmark/rhash_bench --reads=100 --prefill=75 --total-ops=500 --init-size=23 --num-threads=8
../build/benchmark/rhash_bench --inserts=100 --prefill=75 --total-ops=75 --init-size=23 --num-threads=8
../build/benchmark/rhash_bench --erases=100 --prefill=75 --total-ops=75 --init-size=23 --num-threads=8
../build/benchmark/rhash_bench --updates=100 --prefill=75 --total-ops=500 --init-size=23 --num-threads=8
../build/benchmark/rhash_bench --upserts=100 --prefill=25 --total-ops=200 --init-size=23 --num-threads=8
../build/benchmark/rhash_bench --inserts=100  --init-size=4 --total-ops=13107200 --num-threads=8
../build/benchmark/rhash_bench --reads=80  --inserts=20 --init-size=10 --total-ops=4096000 --num-threads=8

# scalability
printf "\nscalability:\n"
# 100% reads
for i in "${threads[@]}"; do
    ../build/benchmark/rhash_bench --reads=100 --init-size=23 --num-threads="$i"
done

# 100% inserts
for i in "${threads[@]}"; do
    ../build/benchmark/rhash_bench --inserts=100 --num-threads="$i"
done

# 100% erases
for i in "${threads[@]}"; do
    ../build/benchmark/rhash_bench --erases=100 --num-threads="$i"
done

# 100% updates
for i in "${threads[@]}"; do
    ../build/benchmark/rhash_bench --updates=100 --prefill=75 --num-threads="$i"
done

# 100% upserts
for i in "${threads[@]}"; do
    ../build/benchmark/rhash_bench --upserts=100  --num-threads="$i"
done

# ubuntu@ubuntu:~/workspace/rbhash/benchmark$ ./rhash_bench.sh
# mix:
# Generate test data done
# Start execuating: table size: 16777216, table capacity: 67108864
# init-size: 25, prefill: 50, total-ops: 23488102, read: 70%, insert: 10%, erase: 10%, update: 5%, upsert: 5%
# End mixing: total ops: 23488102, seed: 3112251692, num_threads: 1, elapse: 2.1549 s, throughput: 1.08999e+07, average latency/op: 91 ns

# Generate test data done
# Start execuating: table size: 16777216, table capacity: 67108864
# init-size: 25, prefill: 50, total-ops: 23488102, read: 70%, insert: 10%, erase: 10%, update: 5%, upsert: 5%
# End mixing: total ops: 23488102, seed: 555133174, num_threads: 2, elapse: 2.13694 s, throughput: 1.09915e+07, average latency/op: 90 ns

# Generate test data done
# Start execuating: table size: 16777215, table capacity: 67108864
# init-size: 25, prefill: 50, total-ops: 23488102, read: 70%, insert: 10%, erase: 10%, update: 5%, upsert: 5%
# End mixing: total ops: 23488102, seed: 3227019622, num_threads: 3, elapse: 1.12659 s, throughput: 2.08489e+07, average latency/op: 47 ns

# Generate test data done
# Start execuating: table size: 16777216, table capacity: 67108864
# init-size: 25, prefill: 50, total-ops: 23488102, read: 70%, insert: 10%, erase: 10%, update: 5%, upsert: 5%
# End mixing: total ops: 23488102, seed: 2641773561, num_threads: 4, elapse: 1.25854 s, throughput: 1.86629e+07, average latency/op: 53 ns

# Generate test data done
# Start execuating: table size: 16777215, table capacity: 67108864
# init-size: 25, prefill: 50, total-ops: 23488102, read: 70%, insert: 10%, erase: 10%, update: 5%, upsert: 5%
# End mixing: total ops: 23488102, seed: 42425417, num_threads: 5, elapse: 0.468541 s, throughput: 5.01303e+07, average latency/op: 19 ns

# Generate test data done
# Start execuating: table size: 16777212, table capacity: 67108864
# init-size: 25, prefill: 50, total-ops: 23488102, read: 70%, insert: 10%, erase: 10%, update: 5%, upsert: 5%
# End mixing: total ops: 23488102, seed: 1261799969, num_threads: 6, elapse: 1.74557 s, throughput: 1.34558e+07, average latency/op: 74 ns

# Generate test data done
# Start execuating: table size: 16777215, table capacity: 67108864
# init-size: 25, prefill: 50, total-ops: 23488102, read: 70%, insert: 10%, erase: 10%, update: 5%, upsert: 5%
# End mixing: total ops: 23488102, seed: 592195991, num_threads: 7, elapse: 0.871672 s, throughput: 2.6946e+07, average latency/op: 37 ns

# Generate test data done
# Start execuating: table size: 16777216, table capacity: 67108864
# init-size: 25, prefill: 50, total-ops: 23488102, read: 70%, insert: 10%, erase: 10%, update: 5%, upsert: 5%
# End mixing: total ops: 23488102, seed: 3069714442, num_threads: 8, elapse: 2.17945 s, throughput: 1.07771e+07, average latency/op: 92 ns


# run once:
# Generate test data done
# Start execuating: table size: 6291456, table capacity: 16777216
# init-size: 23, prefill: 75, total-ops: 41943040, read: 100%, insert: 0%, erase: 0%, update: 0%, upsert: 0%
# End mixing: total ops: 41943040, seed: 3068472169, num_threads: 8, elapse: 3.17084 s, throughput: 1.32277e+07, average latency/op: 75 ns

# Generate test data done
# Start execuating: table size: 6291456, table capacity: 16777216
# init-size: 23, prefill: 75, total-ops: 6291456, read: 0%, insert: 100%, erase: 0%, update: 0%, upsert: 0%
# End mixing: total ops: 6291456, seed: 2905907359, num_threads: 8, elapse: 0.589445 s, throughput: 1.06735e+07, average latency/op: 93 ns

# Generate test data done
# Start execuating: table size: 6291456, table capacity: 16777216
# init-size: 23, prefill: 75, total-ops: 6291456, read: 0%, insert: 0%, erase: 100%, update: 0%, upsert: 0%
# End mixing: total ops: 6291456, seed: 823254724, num_threads: 8, elapse: 0.538544 s, throughput: 1.16823e+07, average latency/op: 85 ns

# Generate test data done
# Start execuating: table size: 6291456, table capacity: 16777216
# init-size: 23, prefill: 75, total-ops: 41943040, read: 0%, insert: 0%, erase: 0%, update: 100%, upsert: 0%
# End mixing: total ops: 41943040, seed: 760738016, num_threads: 8, elapse: 3.49837 s, throughput: 1.19893e+07, average latency/op: 83 ns

# Generate test data done
# Start execuating: table size: 2097152, table capacity: 8388608
# init-size: 23, prefill: 25, total-ops: 16777216, read: 0%, insert: 0%, erase: 0%, update: 0%, upsert: 100%
# End mixing: total ops: 16777216, seed: 959218100, num_threads: 8, elapse: 4.65372 s, throughput: 3.60512e+06, average latency/op: 277 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 16
# init-size: 4, prefill: 0, total-ops: 2097152, read: 0%, insert: 100%, erase: 0%, update: 0%, upsert: 0%
# End mixing: total ops: 2097152, seed: 2958940410, num_threads: 8, elapse: 2.83916 s, throughput: 738652, average latency/op: 1353 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 1024
# init-size: 10, prefill: 0, total-ops: 41943040, read: 80%, insert: 20%, erase: 0%, update: 0%, upsert: 0%
# End mixing: total ops: 41943040, seed: 1271553884, num_threads: 8, elapse: 7.29353 s, throughput: 5.75072e+06, average latency/op: 173 ns


# scalability:
# Generate test data done
# Start execuating: table size: 0, table capacity: 8388608
# init-size: 23, prefill: 0, total-ops: 5872025, read: 100%, insert: 0%, erase: 0%, update: 0%, upsert: 0%
# End mixing: total ops: 5872025, seed: 316976473, num_threads: 1, elapse: 0.452412 s, throughput: 1.29794e+07, average latency/op: 77 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 8388608
# init-size: 23, prefill: 0, total-ops: 5872025, read: 100%, insert: 0%, erase: 0%, update: 0%, upsert: 0%
# End mixing: total ops: 5872025, seed: 215577545, num_threads: 2, elapse: 0.29916 s, throughput: 1.96284e+07, average latency/op: 50 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 8388608
# init-size: 23, prefill: 0, total-ops: 5872025, read: 100%, insert: 0%, erase: 0%, update: 0%, upsert: 0%
# End mixing: total ops: 5872025, seed: 3069696541, num_threads: 3, elapse: 0.0382153 s, throughput: 1.53656e+08, average latency/op: 6 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 8388608
# init-size: 23, prefill: 0, total-ops: 5872025, read: 100%, insert: 0%, erase: 0%, update: 0%, upsert: 0%
# End mixing: total ops: 5872025, seed: 4184328560, num_threads: 4, elapse: 0.170478 s, throughput: 3.44445e+07, average latency/op: 29 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 8388608
# init-size: 23, prefill: 0, total-ops: 5872025, read: 100%, insert: 0%, erase: 0%, update: 0%, upsert: 0%
# End mixing: total ops: 5872025, seed: 1565551070, num_threads: 5, elapse: 0.0350739 s, throughput: 1.67419e+08, average latency/op: 5 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 8388608
# init-size: 23, prefill: 0, total-ops: 5872025, read: 100%, insert: 0%, erase: 0%, update: 0%, upsert: 0%
# End mixing: total ops: 5872025, seed: 3250437028, num_threads: 6, elapse: 0.0292299 s, throughput: 2.00891e+08, average latency/op: 4 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 8388608
# init-size: 23, prefill: 0, total-ops: 5872025, read: 100%, insert: 0%, erase: 0%, update: 0%, upsert: 0%
# End mixing: total ops: 5872025, seed: 39578041, num_threads: 7, elapse: 0.0289618 s, throughput: 2.02751e+08, average latency/op: 4 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 8388608
# init-size: 23, prefill: 0, total-ops: 5872025, read: 100%, insert: 0%, erase: 0%, update: 0%, upsert: 0%
# End mixing: total ops: 5872025, seed: 462069717, num_threads: 8, elapse: 0.3091 s, throughput: 1.89972e+07, average latency/op: 52 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 100%, erase: 0%, update: 0%, upsert: 0%
# End mixing: total ops: 23488102, seed: 2589162507, num_threads: 1, elapse: 2.61656 s, throughput: 8.97671e+06, average latency/op: 111 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 100%, erase: 0%, update: 0%, upsert: 0%
# End mixing: total ops: 23488102, seed: 3188486215, num_threads: 2, elapse: 2.03918 s, throughput: 1.15184e+07, average latency/op: 86 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 100%, erase: 0%, update: 0%, upsert: 0%
# End mixing: total ops: 23488102, seed: 3221025286, num_threads: 3, elapse: 1.59675 s, throughput: 1.47099e+07, average latency/op: 67 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 100%, erase: 0%, update: 0%, upsert: 0%
# End mixing: total ops: 23488102, seed: 1495572288, num_threads: 4, elapse: 1.57439 s, throughput: 1.49188e+07, average latency/op: 67 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 100%, erase: 0%, update: 0%, upsert: 0%
# End mixing: total ops: 23488102, seed: 3546083970, num_threads: 5, elapse: 1.96208 s, throughput: 1.1971e+07, average latency/op: 83 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 100%, erase: 0%, update: 0%, upsert: 0%
# End mixing: total ops: 23488102, seed: 909859925, num_threads: 6, elapse: 2.73719 s, throughput: 8.5811e+06, average latency/op: 116 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 100%, erase: 0%, update: 0%, upsert: 0%
# End mixing: total ops: 23488102, seed: 2760978604, num_threads: 7, elapse: 2.58667 s, throughput: 9.08045e+06, average latency/op: 110 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 100%, erase: 0%, update: 0%, upsert: 0%
# End mixing: total ops: 23488102, seed: 186617151, num_threads: 8, elapse: 2.45498 s, throughput: 9.56751e+06, average latency/op: 104 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 0%, erase: 100%, update: 0%, upsert: 0%
# End mixing: total ops: 23488102, seed: 2786852873, num_threads: 1, elapse: 1.85401 s, throughput: 1.26688e+07, average latency/op: 78 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 0%, erase: 100%, update: 0%, upsert: 0%
# End mixing: total ops: 23488102, seed: 173731066, num_threads: 2, elapse: 1.42743 s, throughput: 1.64548e+07, average latency/op: 60 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 0%, erase: 100%, update: 0%, upsert: 0%
# End mixing: total ops: 23488102, seed: 1225442324, num_threads: 3, elapse: 0.114809 s, throughput: 2.04584e+08, average latency/op: 4 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 0%, erase: 100%, update: 0%, upsert: 0%
# End mixing: total ops: 23488102, seed: 271640767, num_threads: 4, elapse: 0.949708 s, throughput: 2.47319e+07, average latency/op: 40 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 0%, erase: 100%, update: 0%, upsert: 0%
# End mixing: total ops: 23488102, seed: 502556313, num_threads: 5, elapse: 0.119261 s, throughput: 1.96948e+08, average latency/op: 5 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 0%, erase: 100%, update: 0%, upsert: 0%
# End mixing: total ops: 23488102, seed: 405319992, num_threads: 6, elapse: 0.103347 s, throughput: 2.27274e+08, average latency/op: 4 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 0%, erase: 100%, update: 0%, upsert: 0%
# End mixing: total ops: 23488102, seed: 4161420398, num_threads: 7, elapse: 0.101924 s, throughput: 2.30448e+08, average latency/op: 4 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 0%, erase: 100%, update: 0%, upsert: 0%
# End mixing: total ops: 23488102, seed: 642476741, num_threads: 8, elapse: 1.62239 s, throughput: 1.44775e+07, average latency/op: 69 ns

# Generate test data done
# Start execuating: table size: 25165824, table capacity: 67108864
# init-size: 25, prefill: 75, total-ops: 23488102, read: 0%, insert: 0%, erase: 0%, update: 100%, upsert: 0%
# End mixing: total ops: 23488102, seed: 2524975551, num_threads: 1, elapse: 2.16714 s, throughput: 1.08383e+07, average latency/op: 92 ns

# Generate test data done
# Start execuating: table size: 25165824, table capacity: 67108864
# init-size: 25, prefill: 75, total-ops: 23488102, read: 0%, insert: 0%, erase: 0%, update: 100%, upsert: 0%
# End mixing: total ops: 23488102, seed: 4292158070, num_threads: 2, elapse: 1.57322 s, throughput: 1.49299e+07, average latency/op: 66 ns

# Generate test data done
# Start execuating: table size: 25165824, table capacity: 67108864
# init-size: 25, prefill: 75, total-ops: 23488102, read: 0%, insert: 0%, erase: 0%, update: 100%, upsert: 0%
# End mixing: total ops: 23488102, seed: 3766062060, num_threads: 3, elapse: 0.115364 s, throughput: 2.036e+08, average latency/op: 4 ns

# Generate test data done
# Start execuating: table size: 25165824, table capacity: 67108864
# init-size: 25, prefill: 75, total-ops: 23488102, read: 0%, insert: 0%, erase: 0%, update: 100%, upsert: 0%
# End mixing: total ops: 23488102, seed: 2481082193, num_threads: 4, elapse: 1.22646 s, throughput: 1.91511e+07, average latency/op: 52 ns

# Generate test data done
# Start execuating: table size: 25165820, table capacity: 67108864
# init-size: 25, prefill: 75, total-ops: 23488102, read: 0%, insert: 0%, erase: 0%, update: 100%, upsert: 0%
# End mixing: total ops: 23488102, seed: 2124885332, num_threads: 5, elapse: 0.156303 s, throughput: 1.50273e+08, average latency/op: 6 ns

# Generate test data done
# Start execuating: table size: 25165824, table capacity: 67108864
# init-size: 25, prefill: 75, total-ops: 23488102, read: 0%, insert: 0%, erase: 0%, update: 100%, upsert: 0%
# End mixing: total ops: 23488102, seed: 3909102037, num_threads: 6, elapse: 0.17134 s, throughput: 1.37085e+08, average latency/op: 7 ns

# Generate test data done
# Start execuating: table size: 25165819, table capacity: 67108864
# init-size: 25, prefill: 75, total-ops: 23488102, read: 0%, insert: 0%, erase: 0%, update: 100%, upsert: 0%
# End mixing: total ops: 23488102, seed: 2467876624, num_threads: 7, elapse: 1.65769 s, throughput: 1.41692e+07, average latency/op: 70 ns

# Generate test data done
# Start execuating: table size: 25165824, table capacity: 67108864
# init-size: 25, prefill: 75, total-ops: 23488102, read: 0%, insert: 0%, erase: 0%, update: 100%, upsert: 0%
# End mixing: total ops: 23488102, seed: 1463421355, num_threads: 8, elapse: 2.06174 s, throughput: 1.13924e+07, average latency/op: 87 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 0%, erase: 0%, update: 0%, upsert: 100%
# End mixing: total ops: 23488102, seed: 3918875369, num_threads: 1, elapse: 2.48348 s, throughput: 9.45774e+06, average latency/op: 105 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 0%, erase: 0%, update: 0%, upsert: 100%
# End mixing: total ops: 23488102, seed: 3903111575, num_threads: 2, elapse: 2.42719 s, throughput: 9.67707e+06, average latency/op: 103 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 0%, erase: 0%, update: 0%, upsert: 100%
# End mixing: total ops: 23488102, seed: 2117509634, num_threads: 3, elapse: 2.38301 s, throughput: 9.85647e+06, average latency/op: 101 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 0%, erase: 0%, update: 0%, upsert: 100%
# End mixing: total ops: 23488102, seed: 1391874808, num_threads: 4, elapse: 1.67043 s, throughput: 1.40611e+07, average latency/op: 71 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 0%, erase: 0%, update: 0%, upsert: 100%
# End mixing: total ops: 23488102, seed: 2165835880, num_threads: 5, elapse: 0.162636 s, throughput: 1.44421e+08, average latency/op: 6 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 0%, erase: 0%, update: 0%, upsert: 100%
# End mixing: total ops: 23488102, seed: 479644269, num_threads: 6, elapse: 1.94646 s, throughput: 1.20671e+07, average latency/op: 82 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 0%, erase: 0%, update: 0%, upsert: 100%
# End mixing: total ops: 23488102, seed: 2176762393, num_threads: 7, elapse: 2.20396 s, throughput: 1.06572e+07, average latency/op: 93 ns

# Generate test data done
# Start execuating: table size: 0, table capacity: 33554432
# init-size: 25, prefill: 0, total-ops: 23488102, read: 0%, insert: 0%, erase: 0%, update: 0%, upsert: 100%
# End mixing: total ops: 23488102, seed: 2221816509, num_threads: 8, elapse: 2.52124 s, throughput: 9.31609e+06, average latency/op: 107 ns
