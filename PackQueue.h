//
// Created by gas on 04.03.19.
//

#ifndef PACKQUEUE_PACKQUEUE_H
#define PACKQUEUE_PACKQUEUE_H

#include <atomic>
#include <vector>
#include <mutex>
#include <queue>
#include <condition_variable>

#ifdef DEBUG
    #include <time.h>
#endif

template <class T>
class PackQueue;

template <class T>
class PackQueueSender
{
    friend PackQueue<T>;
public:
    ~PackQueueSender() {
        if (!m_pack.empty()) {
            forward();
        }
        m_pack_queue.finish_confirmation();
    }

    /*!
     * lockable
     * @param shipped item
     * @return is pack sent
     */
    bool send(T item) {
        m_pack.push_back(item);

        bool status = (m_pack.size() == m_pack_size || m_pack_size == 0);
        if (status) {
            forward();
        }

        return status;
    }

    PackQueueSender() = delete;

    PackQueueSender(PackQueueSender&) = default;
    PackQueueSender(PackQueueSender&&) noexcept = default;
    PackQueueSender(PackQueueSender const &) = default;
    PackQueueSender& operator=(PackQueueSender const &) = default;
    PackQueueSender& operator=(PackQueueSender&&) noexcept = default;

private:
    PackQueueSender(PackQueue<T>& pack_queue, size_t pack_size):
            m_pack_queue(pack_queue),
            m_pack_size(pack_size) {
    };

    void forward() {
        std::vector<T> package;
        std::swap(package, m_pack);

        m_pack_queue.push(package);
    }

private:
    const size_t m_pack_size;
    std::vector<T> m_pack;
    PackQueue<T>& m_pack_queue;
};

template <class T>
class PackQueueGetter
{
    friend PackQueue<T>;
public:
    ~PackQueueGetter() = default;

    /*!
     * lockable
     * @return next pack
     */
    inline std::vector<T> get() {
        return m_pack_queue.pop();
    }

    PackQueueGetter() = delete;
    PackQueueGetter(PackQueueGetter&) = default;
    PackQueueGetter(PackQueueGetter&&) noexcept = default;
    PackQueueGetter(PackQueueGetter const &) = default;
    PackQueueGetter& operator=(PackQueueGetter const &) = default;
    PackQueueGetter& operator=(PackQueueGetter&&) noexcept = default;

private:
    PackQueueGetter(PackQueue<T>& pack_queue, size_t pack_size):
            m_pack_queue(pack_queue) {
    };

private:
    PackQueue<T>& m_pack_queue;
};

template <class T>
class PackQueue
{
    friend PackQueueSender<T>;
    friend PackQueueGetter<T>;
public:
    /*!
     * @brief constructor for queue. You should use Sender/Getter for modification
     * @param capacity - max size of queue
     * @param push_pack_size - size of send-pack
     * @param pop_pack_size - size of get-pack
     */
    PackQueue(size_t capacity, size_t push_pack_size, size_t pop_pack_size) :
            m_capacity(capacity),
            m_pop_pack_size(pop_pack_size),
            m_push_pack_size(push_pack_size),
            m_unfinished_count(0) {
#ifdef DEBUG
        last_sync_op_time = time(nullptr);
#endif
    }

    PackQueueSender<T>* get_sender() {
#ifdef DEBUG
        fast_debug_async_interval();
#endif

        ++m_unfinished_count;
        return new PackQueueSender<T>(*this, m_push_pack_size);
    }

    PackQueueGetter<T> get_getter() {
        return PackQueueGetter<T>(*this, m_pop_pack_size);
    }

    PackQueue() = delete;
    PackQueue(PackQueue&) = delete;
    PackQueue(PackQueue&&) = delete;
    PackQueue(PackQueue const &) = delete;
    PackQueue& operator=(PackQueue const &) = delete;
    PackQueue& operator=(PackQueue&&) = delete;

private:
    void push(std::vector<T> pack) {
#ifdef DEBUG
        fast_debug_async_interval();
#endif

        {
            std::unique_lock<std::mutex> lck(m_mtx);
            while (m_data.size() >= m_capacity) {
                m_cv_write.wait(lck);

#ifdef DEBUG
                fast_debug_async_interval();
#endif
            }

            for (auto item: pack) {
                m_data.push(item);
            }
        }
        m_cv_read.notify_one();
    }

    std::vector<T> pop() {
        std::vector<T> res;
#ifdef DEBUG
        fast_debug_async_interval();
#endif

        {
            std::unique_lock<std::mutex> lck(m_mtx);

#ifdef DEBUG
            while (
                    m_data.size() < m_pop_pack_size &&
                    debug_async_interval() &&
                    m_unfinished_count != 0
                    ) {
                m_cv.wait(lck);
                fast_debug_async_interval();
            }
#else
            while (m_data.size() < m_pop_pack_size && m_unfinished_count != 0) {
                m_cv_read.wait(lck);
            }
#endif

            for (size_t i = 0; i < m_pop_pack_size && !m_data.empty(); ++i) {
                res.push_back(m_data.front());
                m_data.pop();
            }
        }
        m_cv_write.notify_one();

        return res;
    }

    void finish_confirmation()
    {
#ifdef DEBUG
        fast_debug_async_interval();
#endif

        --m_unfinished_count;
    }

#ifdef DEBUG
    inline void fast_debug_async_interval() {
        printf("PackQueue: async interval: %lf", difftime(time(nullptr), last_sync_op_time));
        last_sync_op_time = time(nullptr);
    }

#pragma optimize("", off)
    bool debug_async_interval() {
        printf("PackQueue: async interval: %lf", difftime(time(nullptr), last_sync_op_time));
        last_sync_op_time = time(nullptr);

        return true;
    }
#pragma optimize("", on)
#endif

private:
    std::condition_variable m_cv_write;
    std::condition_variable m_cv_read;
    std::mutex m_mtx;
    std::queue<T> m_data;

    const size_t m_capacity;
    const size_t m_push_pack_size;
    const size_t m_pop_pack_size;
    std::atomic<size_t> m_unfinished_count;
#ifdef DEBUG
    std::atomic<time_t> last_sync_op_time;
#endif
};

#endif //PACKQUEUE_PACKQUEUE_H