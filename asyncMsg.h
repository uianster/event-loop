#ifndef __ASYNC_MSG_H__
#define __ASYNC_MSG_H__
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
    
    bool isEmpty() const
    {
        return m_queue.empty();
    }
};

#endif //__ASYNC_MSG_H__