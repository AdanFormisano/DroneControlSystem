#ifndef THREADUTILS_H
#define THREADUTILS_H
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <optional>

template <typename T>
struct synced_queue
{
    std::queue<T> queue;
    mutable std::mutex mtx;

    void push(const T& value)
    {
        std::lock_guard lock(mtx);
        queue.push(value);
    }

    std::optional<T> pop()
    {
        std::lock_guard lock(mtx);
        if (queue.empty())
        {
            return std::nullopt;
        }
        T value = queue.front();
        queue.pop();
        return value;
    }

    [[nodiscard]] bool empty() const
    {
        std::lock_guard lock(mtx);
        return queue.empty();
    }

    [[nodiscard]] size_t size() const
    {
        std::lock_guard lock(mtx);
        return queue.size();
    }
};

class ThreadPool
{
public:
    explicit ThreadPool(size_t n_threads);
    ~ThreadPool();

    // Add a task to the thread pool
    void enqueue_task(std::function<void()> task);

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex tasks_mutex;
    std::condition_variable condition;
    bool stop;

    void worker_function();
};

class TickSynchronizer
{
public:
    TickSynchronizer() : active_threads(0), waiting_threads(0) {}

    void thread_started();
    void tick_completed();
    void thread_finished();

private:
    std::mutex mtx;
    std::condition_variable cv;
    int active_threads;
    int waiting_threads;
};

#endif //THREADUTILS_H
