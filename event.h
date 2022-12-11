/**
 * @file event.h
 * @author cuican (cuican@reolinlk.com.cn)
 * @brief
 * @version 0.1
 * @date 2022-12-07
 *
 * Copyright (c) 2022, Reolink.
 *
 */
#ifndef _EVENT_SCHEDULER_H_
#define _EVENT_SCHEDULER_H_
#include <queue>
#include <thread>
#include <mutex>
#include <future>
#include <functional>
#include "asyncMsg.h"
#define _TIME_OUT 1000 // mill sec

static uint64_t get_time()
{
    return std::chrono::system_clock::now().time_since_epoch().count()/1000;
}

/// @brief 构造用户自定义事件
struct userEvent {
  private:
    std::function<void()> work_cb;

  public:
    template <class F, class... Args> void set_cb(F&& f, Args&&... args)
    {
        auto task = std::make_shared<std::packaged_task<typename std::result_of<F(Args...)>::type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        work_cb = [task]() { (*task)(); };
    }

    /// @brief 执行事件回调
    void exec_cb_handle()
    {
        if (nullptr != work_cb)
            work_cb(); // TODO：执行结果没有返回给loop
    }
};


#define EVENT_TYPE_TIME 2
class EventScheduler;

// class Event
// {

// };

class timeEvent{
    public:
    uint64_t _timeout;
    uint64_t time_point;
        uint64_t _repeat;
  private:
    std::function<void()> _work_cb;


    EventScheduler *loop_handle;

  public:
    template <class F, class... Args> 
    void set_cb(uint64_t timeout, uint64_t repeat, F&& f, Args&&... args)
    {
        auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        //std::shared_ptr<std::packaged_task<typename std::result_of<F(Args...)>::type()> task = std::make_shared<std::packaged_task<typename std::result_of<F(Args...)>::type()> >(
        //    std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        //iter();
        _timeout = timeout; 
         _work_cb = [task]() { (task)(); };
        time_point = get_time() + timeout; 
       _repeat = repeat;
    }

    /// @brief 执行事件回调
    void exec_cb_handle()
    {
        if (nullptr != _work_cb)
            _work_cb(); // TODO：执行结果没有返回给loop
    }
    void update_time(uint64_t curtime)
    {
       time_point = curtime + _timeout;
    }
};


/// @brief 事件调度器
class EventScheduler {
    enum LOOP_STAT {
        LOOP_STOPED = 0,
        LOOP_RUNNING = 1,
    };

  public:
    static EventScheduler* createNew()
    {
        return new EventScheduler();
    }

    EventScheduler(const EventScheduler&) = delete;
    EventScheduler(EventScheduler&&) noexcept = delete;
    EventScheduler() noexcept : m_stopFlag(0), m_loopStatus(LOOP_STOPED)
    {
    }
    void init(int type){
        m_eventType = type;
    }

    virtual ~EventScheduler() noexcept
    {
    }

    void addEvent(userEvent&& event)
    {
        m_eventQuque.asyncSend(event);
    }
    
    void addTimeEvent(timeEvent *event)
    {
        m_timer_queue.push(event);
    }

    void run_user_event()
    {
        auto ptr = m_eventQuque.asyncRecv(_TIME_OUT);
        if (ptr) {
            (*ptr).exec_cb_handle();
        } else {
            printf("[startMainLoop] time out!\n");
        }
    }

    void run_time_event(){
        //找到
       // for (;!m_timer_queue.empty();) // 依次执行每一个到达时间点的定时器任务
        {   
            auto ptr = m_timer_queue.top();
            //if (ptr)
            {
                printf("p:[%p],[%lld]-----[%lld]\n", ptr,ptr->time_point ,m_time);
                // 到达定时任务的时间点
                if (ptr->time_point!=0 &&ptr->time_point < m_time)
                {
                   // 更新下次运行时间
                    if (ptr->_repeat)
                    {
                        // 更新timer的时间
                        ptr->update_time(m_time);
                    }else{
                        //ptr->time_point = m_time;
                        ptr->time_point = 0;//m_time;
                    }
                    //执行本次定时任务
                     ptr->exec_cb_handle();
                }
                //else
                  //  break; // 队列首部时间点最早，大于m_time说明整个计数器队列都没有到运行时间
                
                //先停止


            }
            //m_timer_queue.pop();
        }
    }

    void startMainLoop()
    {
        // 防止MainLoop被多次同时启用
        if (LOOP_RUNNING == m_loopStatus) {
            printf("WARN: The main loop are runing!\n");
            return;
        }
        while (m_stopFlag == 0) {
            // 更新时间
            m_time = get_time();

            // 执行定时时间
            run_time_event();
            
            //执行用户实践
            //run_user_event();
            m_loopStatus = LOOP_RUNNING;
        }

        m_loopStatus = LOOP_STOPED;
        // TODO: handle the remian events before stop
    }

    void stopLoop(bool* stop)
    {
        m_stopFlag = 1;
    }
    bool cmp(int left, int right)
    {
        return (left ^ 1) < (right ^ 1);
    };

  private:
  uint64_t m_time;
    volatile std::atomic<bool> m_stopFlag;
    AsyncMsgQ<userEvent> m_eventQuque; // 事件队列

    LOOP_STAT m_loopStatus;
    int m_eventType;

    struct cmp_for_time_event {//重写仿函数
        bool operator()(timeEvent* t1, timeEvent* t2)
        {
            return t1->time_point > t2->time_point;//小顶堆，小的在队首
        }
    };

    std::priority_queue<timeEvent*, std::vector<timeEvent*>, cmp_for_time_event> m_timer_queue;
};

#endif


  
 