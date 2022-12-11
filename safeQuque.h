/**
 * @file safeQueue.h
 * @author cuican (cuican@reolinlk.com.cn)
 * @brief c++11 safe queue
 * @version 0.1
 * @date 2022-10-18
 *
 * Copyright (c) 2022, Reolink.
 *
 */
#ifndef __SAFE_QUEUE__
#define __SAFE_QUEUE__
#pragma once
#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>
#include<chrono>
using clean_cb = void (*)(void* param);
namespace SAFE_STL {
template <typename T> 
class s_queue {
  private:
    mutable std::mutex m_qmtx;
    std::queue<T> m_native_queue;
    std::condition_variable m_cond;
    clean_cb m_cleancb;

  public:
    s_queue() : m_cleancb(nullptr)
    {
        printf("SafeQueue default copy\n");
    }

    s_queue(s_queue const& other)
    {
        std::lock_guard<std::mutex> lk(other.m_qmtx);
        m_native_queue = other.m_native_queue;
        printf("SafeQueue copy copy\n");
    }

    s_queue(s_queue&& other) : m_cleancb(other.m_cleancb)
    {
        std::lock_guard<std::mutex> lk(other.m_qmtx);
        m_native_queue = std::move(other.m_native_queue);
        printf("SafeQueue move copy\n");
    }

    ~s_queue()
    {
        printf("SafeQueue ~SafeQueue:%s\n",m_native_queue.size());
        while (m_native_queue.size()) {
            auto data = std::move(m_native_queue.front());
            m_native_queue.pop();
            // if (m_cleancb) {
            //     m_cleancb(data);
            // }
        }
    }

    // void set_clean_cb(clean_cb func)
    // {
    //     m_cleancb = func;
    // }

    void push(T new_value)
    {
        std::lock_guard<std::mutex> lk(m_qmtx);
        m_native_queue.push(std::move(new_value));
        m_cond.notify_one();
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(m_qmtx);
        if (m_native_queue.empty())
            return std::shared_ptr<T>();
        std::shared_ptr<T> res(std::make_shared<T>(m_native_queue.front()));
        m_native_queue.pop();
        return res;
    }

    std::shared_ptr<T> wait_and_pop(const unsigned int& milli_sec)
    {
        std::unique_lock<std::mutex> lk(m_qmtx);
        if (m_native_queue.size() == 0) {
            if (m_cond.wait_for(lk, std::chrono::milliseconds(milli_sec)) == std::cv_status::timeout) {
                return std::shared_ptr<T>();
            }
        }
        std::shared_ptr<T> res(std::make_shared<T>(m_native_queue.front()));
        m_native_queue.pop();
        return res;
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lk(m_qmtx);
        m_cond.wait(lk, [this] { return !m_native_queue.empty(); });
        std::shared_ptr<T> res(std::make_shared<T>(m_native_queue.front()));
        m_native_queue.pop();
        return res;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lk(m_qmtx);
        return m_native_queue.empty();
    }

    bool front() const
    {
        std::lock_guard<std::mutex> lk(m_qmtx);
        return m_native_queue.front();
    }
};
}; // namespace SAFE_STL
#endif