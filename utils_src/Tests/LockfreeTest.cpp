#include <gtest/gtest.h>
#include <future>
#include "LockfreeQueue.h"


TEST(LockfreeQueueTests, HappyPathTest) {
    utility::LockfreeQueue<int, 1024> queue;
    auto It = queue.getReadIterator();
    queue.push(0);
    queue.push(1);
    queue.push(2);
    queue.push(3);
    EXPECT_EQ(*It, 0);
    EXPECT_EQ(*It, 1);
    auto It2 = It;
    EXPECT_EQ(*It, 2);
    EXPECT_EQ(*It, 3);
    EXPECT_EQ(*It2, 2);
    EXPECT_EQ(*It2, 3);
    queue.push(4);
    EXPECT_EQ(*It, 4);
    EXPECT_EQ(*It2, 4);
}

TEST(LockfreeQueueTests, BufferOverflowTest) {
    utility::LockfreeQueue<int, 4> queue;
    auto It = queue.getReadIterator();
    queue.push(0);
    queue.push(1);
    queue.push(2);
    queue.push(3);
    EXPECT_EQ(*It, 0);
    auto It2 = It;
    EXPECT_EQ(*It, 1);
    EXPECT_EQ(*It, 2);
    EXPECT_EQ(*It, 3);
    queue.push(4);
    queue.push(5);
    EXPECT_EQ(*It, 4);
    EXPECT_EQ(*It, 5);
    EXPECT_FALSE(*It2);
}

TEST(LockfreeQueueTests, SingleProducerSingleConsumer) {
    utility::LockfreeQueue<size_t, 1<<12> queue;
    constexpr size_t numberOfElements = 100'000;
    std::future<void> consumer = std::async(std::launch::async, [It = queue.getReadIterator()]() mutable {
        for(size_t i = 0; i < numberOfElements; ++i) {
            EXPECT_EQ(i, *It);
        }
    });
    std::future<void> producer = std::async(std::launch::async, [&queue](){
        for(size_t i = 0; i < numberOfElements; ++i) {
            queue.push(i);
        }
    });
    consumer.get();
    producer.get();
}

TEST(LockfreeQueueTests, SingleProducerMultipleConsumer) {
    utility::LockfreeQueue<size_t, 1<<12> queue;
    constexpr size_t numberOfElements = 100'000;
    constexpr size_t numberOfConsumer = 4;
    std::vector<std::future<void>> consumers;
    consumers.reserve(numberOfConsumer);
    for(size_t i = 0; i < numberOfConsumer; ++i) { 
        consumers.emplace_back(std::async(std::launch::async, [It = queue.getReadIterator()]() mutable {
            for(size_t i = 0; i < numberOfElements; ++i) {
                EXPECT_EQ(i, *It);
            }
        }));
    }
    std::future<void> producer = std::async(std::launch::async, [&queue](){
        for(size_t i = 0; i < numberOfElements; ++i) {
            queue.push(i);
        }
    });
    for (auto &consumer : consumers) {
        consumer.get();
    }
    producer.get();
}

TEST(LockfreeQueueTests, SingleProducerSlowMultipleConsumer) {
    utility::LockfreeQueue<size_t, 1<<12> queue;
    constexpr size_t numberOfElements = 100'000;
    constexpr size_t numberOfConsumer = 4;
    std::vector<std::future<void>> consumers;
    consumers.reserve(numberOfConsumer);
    for(size_t i = 0; i < numberOfConsumer; ++i) { 
        consumers.emplace_back(std::async(std::launch::async, [It = queue.getReadIterator()]() mutable {
            for(size_t i = 0; i < numberOfElements; ++i) {
                auto x = *It;
                if (!x) {
                    return;
                }
                EXPECT_EQ(*x, i);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            FAIL();
        }));
    }
    std::future<void> producer = std::async(std::launch::async, [&queue](){
        for(size_t i = 0; i < numberOfElements; ++i) {
            queue.push(i);
        }
    });
    for (auto &consumer : consumers) {
        consumer.get();
    }
    producer.get();
}
