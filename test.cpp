#include "stdio.h"
#include "event.h"
#include <thread>
#include <string>
static void triggerCallback(void* args)
{
    int* p = (int*) args;
    printf("TriggerEvent:%d\n", *p);
}
EventScheduler* g_scheduler;

#define TEST_DESCRIBLE(a) \
    printf("TEST for %s\n",a);

int main()
{
    // 全局，对所有线程可见
    g_scheduler = EventScheduler::createNew();

    // test1: 单次触发
    std::thread t_producer1([&] {
        auto cb_other = [](std::string data) { printf("[test1][one_time_event] :%s\n", data.c_str()); };
        userEvent one_time_event;
        one_time_event.set_cb(cb_other, "I'm a one_time event");
        
        g_scheduler->addEvent(std::move(one_time_event));
    });

    //test1 ：多次触发
    std::thread t_producer2([&] {
        auto cb_other = [](int data) { printf("[test2][cb_other] a new peer data:%d\n", data); };
        int i = 0;
        while (i < 10) {
            userEvent otherEvent;
            otherEvent.set_cb(cb_other, i++);
            g_scheduler->addEvent(std::move(otherEvent));
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    });

    // test1：嵌套事件测试
    std::thread t_producer3([&] {
        auto cb_accept = [](int fd) {
            printf("[test3][cb_accept] accept a new peer fd:%d\n", fd);
            //事件的回调函数
            auto cb_read = [](std::string str) {
                static int index;
                printf("[test4][cb_read] recv data:%s index:%d\n", str.c_str(), index++);
            };

            userEvent readEvent;
            readEvent.set_cb(cb_read, "hello world!");
            g_scheduler->addEvent(std::move(readEvent));
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        };

        for (int i = 0; i < 10; i++) {
            userEvent triggerEvent;
            triggerEvent.set_cb(cb_accept, i);
            g_scheduler->addEvent(std::move(triggerEvent));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    TEST_DESCRIBLE("Call <startMainLoop> multiple times")
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::thread t_producer4([&] {
            g_scheduler->startMainLoop();
            g_scheduler->startMainLoop();
            g_scheduler->startMainLoop();
        });
        t_producer4.detach();
    }

    // 事件处理线程T0
    g_scheduler->startMainLoop();


    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    }

    if (t_producer1.joinable()) {
        t_producer1.join();
    }
    if (t_producer2.joinable()) {
        t_producer2.join();
    }
    if (t_producer3.joinable()) {
        t_producer3.join();
    }

    return 0;
}