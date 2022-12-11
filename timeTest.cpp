#include <stdio.h>
#include "event.h"
#include <thread>
#include <string>
static void triggerCallback(void *args)
{
    int *p = (int *)args;
    printf("TriggerEvent:%d\n", *p);
}
 EventScheduler *g_scheduler;

#define TEST_DESCRIBLE(a) \
    printf("TEST for %s\n", a);

int main()
{
    // 全局，对所有线程可见
    g_scheduler = EventScheduler::createNew();
    // test1: 单次触发
    std::thread test_timer1([&]
                            {
        auto cb_other = [](std::string data) { printf("[test1][one_time_event] :%s\n", data.c_str()); };
       // userEvent one_time_event;
       timeEvent *time_event = new timeEvent; //FIXME：还没有释放
       time_event->set_cb(10, 1, cb_other,"234");
       
         g_scheduler->addTimeEvent(time_event);
       
        });
 
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    g_scheduler->startMainLoop();

    if(test_timer1.joinable())
    {

        test_timer1.join();
    }
    
    return 0;
}