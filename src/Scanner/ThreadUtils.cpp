#include "ThreadUtils.h"

#include <iostream>


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

void ThreadSemaphore::add_thread()
{
    std::lock_guard lock(mtx);
    ++active_threads;
    // std::cout << "Thread ID: " << std::this_thread::get_id()
    //           << " - Active threads: " << active_threads << std::endl;
}

void ThreadSemaphore::remove_thread()
{
    std::lock_guard lock(mtx);
    --active_threads;
}

void ThreadSemaphore::sync()
{
    {
        std::lock_guard lock(mtx);
        ++waiting_threads;
        // std::cout << "Thread ID: " << std::this_thread::get_id()
        //           << " - Active threads: " << active_threads
        //           << " - Waiting threads: " << waiting_threads << std::endl;
        if (waiting_threads == active_threads)
        {
            // std::cout << "Thread ID: " << std::this_thread::get_id()
            //       << " - Active threads: " << active_threads
            //       << " - Waiting threads: " << waiting_threads << std::endl;
            // std::this_thread::sleep_for(std::chrono::milliseconds(5));
            sem.release(active_threads);
            waiting_threads = 0;
        }
    }
    // std::cout << "Thread ID: " << std::this_thread::get_id()
    //               << " - Active threads: " << active_threads
    //               << " - Waiting threads: " << waiting_threads << std::endl;
    sem.acquire();
}
