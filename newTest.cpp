#include <iostream>
#include <functional>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <unistd.h> // for close()
#include <sys/epoll.h> // for epoll
#include <unordered_map>
#include <atomic>
#include <stdexcept>

class TaskScheduler {
public:
    enum class TaskType {
        Timer,
        Network,
        OneTime
    };

    TaskScheduler() : stop(false), epoll_fd(-1) {}

    ~TaskScheduler() {
        if (epoll_fd != -1) {
            close(epoll_fd);
        }
    }

    // 添加任务到调度器
    void addTask(std::function<void()> task, std::chrono::milliseconds delay, TaskType type) {
        auto time = std::chrono::steady_clock::now() + delay;
        tasks.push({task, time, delay, type});
    }

    // 停止任务调度器
    void stopScheduler() {
        stop = true;
        if (eventHandlingThread.joinable()) {
            eventHandlingThread.join();
        }
    }

private:
    struct ScheduledTask {
        std::function<void()> task;
        std::chrono::steady_clock::time_point time;
        std::chrono::milliseconds delay;
        TaskType type;
        bool operator>(const ScheduledTask& other) const {
            return time > other.time;
        }
    };

    std::priority_queue<ScheduledTask, std::vector<ScheduledTask>, std::greater<>> tasks;
    std::atomic<bool> stop;
    std::thread eventHandlingThread;
    int epoll_fd;

    // 事件处理循环
    void schedulerLoop() {
        std::cout << "Scheduler loop started\n";
        epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            throw std::runtime_error("Failed to create epoll fd");
        }
        
        while (!stop) {
            // 计算下一个任务的超时时间
            auto now = std::chrono::steady_clock::now();
            auto nextTaskTime = tasks.empty() ? now + std::chrono::hours(24) : tasks.top().time;

            while (!tasks.empty() && tasks.top().time <= now) {
                auto task = tasks.top().task;
                auto type = tasks.top().type;
                auto delay = tasks.top().delay;
                tasks.pop();
                if (type == TaskType::Timer || type == TaskType::OneTime) {
                    try {
                        task(); // 执行定时任务或单次执行任务
                        if(type == TaskType::Timer)
                        {
                            addTask(task, delay, type);
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Exception caught: " << e.what() << std::endl;
                    }
                }
            }

            // 更新下一个任务的超时时间
            nextTaskTime = tasks.empty() ? now + std::chrono::hours(24) : tasks.top().time;

            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(nextTaskTime - now);
            int timeout = duration.count();

            // 使用 epoll_wait() 等待事件
            struct epoll_event events[64];
            int nfds = epoll_wait(epoll_fd, events, 64, timeout);
            if (nfds == -1) {
                // 处理 epoll_wait() 错误
                std::cerr << "Error in epoll_wait()\n";
                throw std::runtime_error("Error in epoll_wait()");
            } else if (nfds == 0) {
                //std::cout << "No events\n";
            } else {
                std::cout << "Events occurred\n";
                // 处理网络事件
                for (int i = 0; i < nfds; ++i) {
                    auto fd = events[i].data.fd;
                    auto task = tasks.top().task;
                    try {
                        task(); // 执行网络任务
                    } catch (const std::exception& e) {
                        std::cerr << "Exception caught: " << e.what() << std::endl;
                    }
                }
            }
        }

        std::cout << "Scheduler loop stopped\n";
        // 关闭 epoll 文件描述符
        close(epoll_fd);
    }
    friend void startTaskScheduler(TaskScheduler& scheduler);

};

// 外部启动任务调度器的函数
void startTaskScheduler(TaskScheduler& scheduler) {
    scheduler.schedulerLoop();
}
int main() {
    
    // one thread
    {
        TaskScheduler scheduler;

        // 添加一些任务到调度器
        scheduler.addTask([](){ std::cout << "Timer Task 1 executed\n"; }, std::chrono::milliseconds(1000), TaskScheduler::TaskType::Timer);
        scheduler.addTask([](){ std::cout << "Timer Task 2 executed\n"; }, std::chrono::milliseconds(5000), TaskScheduler::TaskType::Timer);

        int socket_fd = 123; // 假设为某个套接字描述符
        scheduler.addTask([&socket_fd]() {
            // 处理网络事件
            std::cout << "Network Task executed on socket " << socket_fd << "\n";
            // 关闭套接字
            close(socket_fd);
        }, std::chrono::milliseconds(0), TaskScheduler::TaskType::Network);

        
        scheduler.addTask([]()
        { std::cout << "One-time Task executed\n"; 
        }, std::chrono::milliseconds(3000), TaskScheduler::TaskType::OneTime);

        startTaskScheduler(scheduler);

        // 停止调度器
        scheduler.stopScheduler();
    }


    return 0;
}
