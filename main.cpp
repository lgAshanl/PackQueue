#include <iostream>
#include <thread>
#include <vector>
#include <gtest/gtest.h>
#include "PackQueue.h"

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(PackQueue, simple) {
    size_t send_size = 1000;
    size_t get_size = 1000;
    size_t capacity = UINT64_MAX;
    PackQueue<uint64_t> queue(capacity, send_size, get_size);
    PackQueueSender<uint64_t>* sender = queue.get_sender();
    PackQueueGetter<uint64_t> getter = queue.get_getter();

    size_t upper_bound = 1000000000; //~8GB RAM
    // 100889ms R5 2600 4GHZ / 3400MHZ 14-15-14-28

    for (size_t i = 0; i < upper_bound; ++i) {
        sender->send(i);
    }
    delete sender;

    for (size_t i = 0; i < upper_bound;) {
        auto pack = getter.get();

        for (auto item: pack) {
            ASSERT_EQ(item, i);
            ++i;
        }
    }
}

TEST(PackQueue, simple_async) {
    size_t send_size = 1000;
    size_t get_size = 1000;
    size_t capacity = UINT64_MAX;
    PackQueue<uint64_t> queue(capacity, send_size, get_size);

    size_t upper_bound = 500000000; //~8GB RAM
    // 162942ms R5 2600 4GHZ / 3400MHZ 14-15-14-28

    size_t n_threads = 2;
    std::vector<std::thread> send_threads;
    for (size_t thread_index = 0; thread_index < n_threads; ++thread_index) {
        PackQueueSender<uint64_t>* sender = queue.get_sender();
        send_threads.emplace_back(
            [](PackQueueSender<uint64_t>* sender, size_t upper_bound) -> void {
                for (size_t i = 0; i < upper_bound; ++i) {
                    sender->send(i);
                }

                delete sender;
            },
            sender,
            upper_bound
        );
    }

    std::vector<std::thread> get_threads;
    for (size_t thread_index = 0; thread_index < n_threads; ++thread_index) {
        PackQueueGetter<uint64_t> getter = queue.get_getter();
        get_threads.emplace_back(
                [](PackQueueGetter<uint64_t> getter, size_t upper_bound) -> void {
                    auto pack = getter.get();
                    while (!pack.empty()) {
                        for (auto item: pack) {
                            ASSERT_TRUE(0 <= item && item <= upper_bound);
                        }
                        pack = getter.get();
                    }
                },
                getter,
                upper_bound
        );
    }

    for (std::thread& thread: send_threads) {
        thread.join();
    }

    for (std::thread& thread: get_threads) {
        thread.join();
    }
}