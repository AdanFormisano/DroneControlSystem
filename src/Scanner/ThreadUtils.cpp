#include "ThreadUtils.h"

#include <iostream>

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
    std::cout << "Thread ID: " << std::this_thread::get_id() << " started. Active threads: " << active_threads << std::endl;
}

void TickSynchronizer::tick_completed()
{
    std::unique_lock lock(mtx);  // Hold the lock for the entire block
    ++waiting_threads;

    std::cout << "Thread ID: " << std::this_thread::get_id()
              << " - Active threads: " << active_threads
              << " - Waiting threads: " << waiting_threads << std::endl;

    // If all active threads are waiting, reset waiting_threads and notify everyone
    if (waiting_threads == active_threads)
    {
        std::cout << "Thread ID: " << std::this_thread::get_id()
                  << " - All threads have reached waiting state. Resetting waiting_threads." << std::endl;

        // Log waiting_threads and active_threads before resetting
        std::cout << "Before reset - Waiting threads: " << waiting_threads
                  << ", Active threads: " << active_threads << std::endl;

        waiting_threads = 0;  // Reset waiting_threads

        // Log after reset
        std::cout << "After reset - Waiting threads: " << waiting_threads
                  << ", Active threads: " << active_threads << std::endl;

        cv.notify_all();  // Notify all threads to proceed
    }
    else
    {
        // Wait for other threads to reach the waiting state
        std::cout << "Thread ID: " << std::this_thread::get_id()
                  << " - Waiting for other threads to complete." << std::endl;

        // Wait until the condition that waiting_threads == 0 is met
        cv.wait(lock, [this] {
            std::cout << "Thread ID: " << std::this_thread::get_id()
                      << " - Checking condition: waiting_threads == " << waiting_threads << std::endl;
            return waiting_threads == 0;
        });

        // Logging after thread is released
        std::cout << "Thread ID: " << std::this_thread::get_id()
                  << " has been released." << std::endl;
    }
}

void TickSynchronizer::thread_finished()
{
    std::lock_guard<std::mutex> lock(mtx);
    --active_threads;
    std::cout << "Thread ID: " << std::this_thread::get_id()
              << " finished. Active threads remaining: " << active_threads << std::endl;

    if (active_threads == 0)
    {
        std::cout << "Thread ID: " << std::this_thread::get_id()
                  << " - Last active thread finished. Notifying all." << std::endl;
        cv.notify_all();
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
            sem.release(active_threads);
            waiting_threads = 0;
        }
    }
    sem.acquire();
}
