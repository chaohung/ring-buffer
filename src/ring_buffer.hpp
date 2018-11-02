#ifndef HSU_RING_BUFFER_HPP
#define HSU_RING_BUFFER_HPP

#include <unistd.h>

#include <atomic>
#include <cstdio>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace hsu {

template <typename T>
class ring_buffer {
public:
    ring_buffer(T* data, size_t capacity) :
        buf_(data), write_index_(0), read_index_(0), size_(0), capacity_(capacity),
        read_interrupt_(false), read_mtx_(), write_mtx_(), read_cv_(), write_cv_() {}

    void write(T const& data) {
        std::unique_lock<std::mutex> lock(write_mtx_);
        write_cv_.wait(lock, [&] { return write_able(); });
        buf_[write_index_] = data;
        write_index_ = ++write_index_ % capacity_;
        size_++;
        read_cv_.notify_one();
    }

    bool read(T* data) {
        std::unique_lock<std::mutex> lock(read_mtx_);
        read_cv_.wait(lock, [&] { return read_able() || read_interrupt_; });
        if (read_interrupt_) {
            read_interrupt_ = false;
            return false;
        }
        *data = buf_[read_index_];
        read_index_ = ++read_index_ % capacity_;
        size_--;
        write_cv_.notify_one();
        return true;
    }

    void read_interrupt() {
        read_interrupt_ = true;
        read_cv_.notify_one();
    }

private:
    inline bool write_able() const {
        return size_ < capacity_;
    }
    inline bool read_able() const {
        return size_ > 0;
    }

private:
    T* buf_;
    int write_index_;
    int read_index_;
    std::atomic<size_t> size_;
    size_t capacity_;
    bool read_interrupt_;
    std::mutex read_mtx_;
    std::mutex write_mtx_;
    std::condition_variable read_cv_;
    std::condition_variable write_cv_;
};

} // end of namespace hsu

#endif
