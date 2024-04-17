// This file is part of usync library
// Copyright 2021-2022 Andrei Ilin <ortfero@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once


#include <atomic>
#include <shared_mutex>
#include <thread>
#include <utility>
#include <vector>

#if defined(_MSC_VER)
#    include <intrin.h>
#endif


namespace usync {


    static constexpr std::size_t cacheline_size = 64;


    class no_lock {
    public:
        no_lock() noexcept = default;
        no_lock(no_lock const&) noexcept = delete;
        no_lock& operator=(no_lock const&) noexcept = delete;

        bool try_lock() noexcept { return true; }
        void unlock() noexcept {}
        void lock() noexcept {}
        bool try_lock_shared() noexcept { return true; }
        void unlock_shared() noexcept {}
        void lock_shared() noexcept {}

    };   // no_lock


    class spinlock {
    public:
        spinlock() noexcept = default;
        spinlock(spinlock const&) noexcept = delete;
        spinlock& operator=(spinlock const&) noexcept = delete;


        bool try_lock() noexcept {
            if(flag_.load(std::memory_order_relaxed))
                return false;

            return !flag_.exchange(true, std::memory_order_acquire);
        }


        void unlock() noexcept {
            flag_.store(false, std::memory_order_release);
        }


        void lock() noexcept {
            while(!try_lock())
                std::this_thread::yield();
        }


        bool try_lock_shared() noexcept { return try_lock(); }


        void unlock_shared() noexcept { unlock(); }


        void lock_shared() noexcept { lock(); }


    private:
        alignas(cacheline_size) std::atomic_bool flag_ {false};

    };   // spinlock


    class shared_spinlock {
    public:
        shared_spinlock() noexcept = default;
        shared_spinlock(shared_spinlock const&) noexcept = delete;
        shared_spinlock& operator=(shared_spinlock const&) noexcept = delete;


        bool try_lock() noexcept {
            if(data_.writer.load(std::memory_order_relaxed))
                return false;

            if(data_.writer.exchange(true, std::memory_order_acquire))
                return false;

            while(data_.readers.load(std::memory_order_relaxed) > 0)
                relax();

            return true;
        }


        void unlock() noexcept {
            data_.writer.store(false, std::memory_order_release);
        }


        void lock() noexcept {
            while(!try_lock())
                std::this_thread::yield();
        }


        bool try_lock_shared() noexcept {
            if(data_.writer.load(std::memory_order_relaxed))
                return false;

            data_.readers.fetch_add(1, std::memory_order_relaxed);

            bool expected = true;
            if(data_.writer.compare_exchange_weak(expected,
                                                  true,
                                                  std::memory_order_relaxed)) {
                data_.readers.fetch_sub(1, std::memory_order_relaxed);
                return false;
            }

            return true;
        }


        void unlock_shared() noexcept {
            data_.readers.fetch_sub(1, std::memory_order_relaxed);
        }


        void lock_shared() noexcept {
            while(!try_lock_shared())
                std::this_thread::yield();
        }

    private:
        alignas(cacheline_size) struct data {
            std::atomic_bool writer {false};
            std::atomic_uint readers {0};
        } data_;


        static void relax() {
#if defined(_MSC_VER)

#    if defined(_M_AMD64) || defined(_M_IX86)
            _mm_pause();
#    elif defined(_M_ARM)
            __yield();
#    endif   // _M_IX86

#else

#    if defined(__x86_64__) || defined(__i386__)
            __asm__ __volatile__("pause");
#    elif defined(__arm__)
            __asm__ __volatile__("yield");
#    endif   // __i386__

#endif   // _MSC_VER
        }
    };   // shared_spinlock


    template<typename T, typename L = spinlock>
    struct synchronized {
        using resource_type = T;

        struct unique_access {
            unique_access() = delete;
            unique_access(unique_access const&) = delete;
            unique_access& operator=(unique_access const&) = delete;

            unique_access(synchronized& owner) noexcept
                : guard_(owner.lock_), resource_(owner.resource_) {}

            T& operator*() const noexcept { return resource_; }
            T* operator->() const noexcept { return &resource_; }
            template<typename F> void run(F&& f) { f(resource_); }

        private:
            std::unique_lock<L> guard_;
            T& resource_;

        };   // unique_access


        struct shared_access {
            shared_access() = delete;
            shared_access(shared_access const&) = delete;
            shared_access& operator=(shared_access const&) = delete;

            shared_access(synchronized const& owner) noexcept
                : guard_(owner.lock_), resource_(owner.resource_) {}

            T const& operator*() const noexcept { return resource_; }
            T const* operator->() const noexcept { return &resource_; }
            template<typename F> void run(F&& f) { f(resource_); }

        private:
            std::shared_lock<L> guard_;
            T const& resource_;

        };   // shared_access


        synchronized() = default;
        synchronized(synchronized const&) = delete;
        synchronized& operator=(synchronized const&) = delete;
        synchronized(synchronized&&) = default;
        synchronized& operator=(synchronized&&) = default;

        template<typename... Args>
        synchronized(Args&&... args): resource_(std::forward<Args>(args)...) {}

    private:
        mutable L lock_;
        T resource_;

    };   // synchronized


    template<typename T>
    class promise {
        T value_;
        std::atomic_flag event_;

    public:

        promise() = default;
        promise(promise const&) = delete;
        promise& operator = (promise const&) = delete;

        promise& operator = (T const& value) {
            value_ = value;
            event_.test_and_set(std::memory_order_relaxed);
            event_.notify_one();
            return *this;
        }


        promise& operator = (T&& value) {
            value_ = std::move(value);
            event_.test_and_set(std::memory_order_relaxed);
            event_.notify_one();
            return *this;
        }


        T const& await() const& noexcept {
            event_.wait(false, std::memory_order_relaxed);
            return value_;            
        }


        T&& await() && noexcept {
            event_.wait(false, std::memory_order_relaxed);
            return std::move(value_);
        }


        void clear() {
            event_.clear(std::memory_order_relaxed);
        }

    }; // promise


    template<class T> class pool {

        std::vector<T> recycled_;
        std::list<T> in_use_;

    public:

        using pointer = typename std::list<T>::iterator;

        pool() = default;
        pool(pool const&) = delete;
        pool& operator = (pool const&) = delete;
        pool(pool&&) = default;
        pool& operator = (pool&&) = default;

        pointer remake() {
            if(recycled_.empty()) {
                in_use_.push_front(T{});
            } else {
                in_use_.push_front(std::move(recycled_.back()));
                recycled_.pop_back();
            }
            return in_use_.begin();
        }


        void recycle(pointer it) {
            recycled_.push_back(std::move(*it));
            in_use_.erase(it);
        }

    }; // pool

    template<class T> using pool_pointer = typename pool<T>::pointer;
    template<class T> using synchronized_pool = synchronized<pool<T>>;


}   // namespace usync
