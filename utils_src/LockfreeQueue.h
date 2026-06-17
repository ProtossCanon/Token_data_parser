#include <array>
#include <atomic>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>

namespace utility {

namespace spin_lock {

    static constexpr std::size_t writeLocked = std::numeric_limits<size_t>::max();

    class SpinLockWrite {
    public:
        SpinLockWrite(std::atomic<size_t> &lock) : lock(lock) {
            size_t expected = 0;
            while(!lock.compare_exchange_strong(expected, writeLocked)) {
                expected = 0;
            }
        }
        ~SpinLockWrite() {
            lock = 0;
        }
    private:
        std::atomic<size_t> &lock;
    };

    class SpinLockRead {
    public:
        SpinLockRead(std::atomic<size_t> &lock) : lock(lock) {
            size_t desired;
            size_t expected = lock.load();
            do {
                if (expected == writeLocked) {
                    expected = 0;
                } 
                desired = expected + 1;
            } while(!lock.compare_exchange_strong(expected, desired));
        }
        ~SpinLockRead() {
            size_t desired;
            size_t expected = lock.load();
            do {
                desired = expected - 1;
            } while(!lock.compare_exchange_strong(expected, desired));
        }
    private:
        std::atomic<size_t> &lock;
    };
} // namespace spin_lock

template<typename T, std::size_t BufferSize>
class LockfreeQueue {
public:
    LockfreeQueue() : currentEvent(0), head(0) {
        for (auto &lock : locks) {
            lock = 0;
        }
    }

    void push(T data) {
        {
            [[maybe_unused]] spin_lock::SpinLockWrite guard(locks[head]);
            circularBuffer[head] = BufferData{std::make_shared<T>(data), currentEvent};
        }
        ++currentEvent;
        head = (head + 1) % BufferSize;
    }

    class ReadIterator {
    public:
        ReadIterator() = delete;
        ReadIterator(const ReadIterator&) = default;
        ReadIterator(ReadIterator&&) = default;
        ReadIterator &operator=(const ReadIterator&) = default;
        ReadIterator &operator=(ReadIterator&&) = default;

        std::optional<T> operator*() {
            while (eventNumber == queue.currentEvent) {}
            std::shared_ptr<T> ptr;
            {
                [[maybe_unused]] spin_lock::SpinLockRead guard(queue.locks[pos]);
                if (queue.circularBuffer[pos].eventNumber != eventNumber)
                    return std::nullopt;
                ptr = queue.circularBuffer[pos].data;
            }
            ++eventNumber;
            pos = (pos + 1) % BufferSize;
            return *ptr;
        }
    private:
        LockfreeQueue &queue;
        size_t eventNumber;
        size_t pos;

        ReadIterator(LockfreeQueue &queue) : queue(queue), eventNumber(queue.currentEvent), pos(queue.head) {}

        friend LockfreeQueue<T, BufferSize>;
    };

    ReadIterator getReadIterator() {
        return ReadIterator(*this);
    }

private:

    struct BufferData {
        std::shared_ptr<T> data;
        size_t eventNumber;
    };


    std::array<BufferData, BufferSize> circularBuffer;
    std::array<std::atomic<size_t>, BufferSize> locks;
    std::atomic<size_t> currentEvent;
    size_t head;
};

} // namespace utility
