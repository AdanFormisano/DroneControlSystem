#include "ThreadUtils.h"

#include "spdlog/spdlog.h"

ThreadPool::ThreadPool(size_t n_threads) : stop(false)
{
    for (size_t i = 0; i < n_threads; ++i)
    {
        workers.emplace_back([this] { this->worker_function(); });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::lock_guard<std::mutex> lock(tasks_mutex);
        stop = true;
    }
    condition.notify_all();

    for (std::thread& worker : workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
}

void ThreadPool::enqueue_task(std::function<void()> task)
{
    {
        std::lock_guard lock(tasks_mutex);
        tasks.push(task);
    }
    condition.notify_one();
}

void ThreadPool::worker_function()
{
    while (true)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(tasks_mutex);
            this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
            if (this->stop && this->tasks.empty())
            {
                return;
            }
            task = std::move(this->tasks.front());
            this->tasks.pop();
        }
        task();
    }
}

void TickSynchronizer::thread_started()
{
    std::lock_guard lock(mtx);
    ++active_threads;
}

void TickSynchronizer::tick_completed()
{
    std::unique_lock lock(mtx);
    ++waiting_threads;

    // spdlog::info("Active threads {} - Waiting threads {}", active_threads, waiting_threads);

    if (waiting_threads == active_threads)
    {
        waiting_threads = 0;
        // spdlog::info("Active threads {} - Waiting threads {}", active_threads, waiting_threads);
        cv.notify_all();
    } else
    {
        // spdlog::info("Active threads {} - Waiting threads {}", active_threads, waiting_threads);
        cv.wait(lock, [this] { return waiting_threads == 0; });
    }
}

void TickSynchronizer::thread_finished()
{
    std::lock_guard lock(mtx);
    --active_threads;
}
