#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <type_traits>
#include <atomic>

namespace graphflow::concurrency {

/**
 * @brief Lightweight, low-overhead Task-parallel Thread Pool.
 */
class ThreadPool {
public:
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency()) {
        threads = std::max(size_t{1}, threads);
        stop_.store(false);
        workers_.reserve(threads);

        for (size_t i = 0; i < threads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this] {
                            return stop_.load() || !tasks_.empty();
                        });

                        if (stop_.load() && tasks_.empty()) {
                            return;
                        }

                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        stop_.store(true);
        condition_.notify_all();
        for (std::thread& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            [f = std::forward<F>(f), ...args = std::forward<Args>(args)]() mutable {
                return f(std::forward<Args>(args)...);
            }
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stop_.load()) {
                throw std::runtime_error("submit called on stopped ThreadPool");
            }
            tasks_.emplace([task]() { (*task)(); });
        }
        condition_.notify_one();
        return res;
    }

    [[nodiscard]] size_t size() const noexcept {
        return workers_.size();
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};
};

} // namespace graphflow::concurrency
