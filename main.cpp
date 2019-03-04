#include <gtest/gtest.h>
#include <iostream>
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
    PackQueueSender<uint64_t> sender = queue.get_sender();
    PackQueueGetter<uint64_t> getter = queue.get_getter();

    size_t upper_bound = 1000000000; //~8GB RAM
    for (size_t i = 0; i < upper_bound; ++i) {
        sender.send(i);
    }

    for (size_t i = 0; i < upper_bound;) {
        auto pack = getter.get();

        for (auto item: pack) {
            ASSERT_EQ(item, i);
            ++i;
        }
    }

}