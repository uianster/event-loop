/**
 * @file asyncMsg.h
 * @author cuican (cuican@reolinlk.com.cn)
 * @brief brief Asynchronous message sending and receiving template between the two threads
 * @version 0.1
 * @date 2022-10-13
 *
 * Copyright (c) 2022, Reolink.
 *
 */

#ifndef __ASYNC_H__
#define __ASYNC_H__
#pragma once
#include "safeQuque.h"

using mill_sec = unsigned int;
template <class T> class AsyncMsgQ {
  private:
    SAFE_STL::s_queue<T> m_queue;
  public:
    // 发数据，缓存至队列中
    bool asyncSend(const T& data)
    {
        m_queue.push(std::move(data));
        return true;
    }

    std::shared_ptr<T> asyncRecv(const mill_sec& time)
    {
        if (time > 0) { // timeout
            return m_queue.wait_and_pop(time);
        } else if (time == 0) { // unblock
            return m_queue.try_pop();
        } else { // block
            return m_queue.wait_and_pop();
        }
    }
};

#endif //__ASYNC_SENDER_H__