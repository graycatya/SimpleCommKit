#pragma once
#include "BleBackend.h"
#include <algorithm>
#include <type_traits>
#include <condition_variable>
#include <deque>
#include <future>
#include <mutex>
#include <thread>

namespace SimpleCommKit::Ble {
// The worker owns State rather than the executor. Destruction from its own
// callback closes the queue and detaches safely; State lives until run returns.
class SerialExecutor {
public:
    SerialExecutor();
    ~SerialExecutor();
    SerialExecutor(const SerialExecutor&) = delete;
    SerialExecutor& operator=(const SerialExecutor&) = delete;
    bool post(std::function<void()> work);
    bool isCurrent() const;
    void close();
    void finish();
private:
    struct State {
        std::mutex mutex;
        std::condition_variable changed;
        std::deque<std::function<void()>> queue;
        bool closed = false;
    };
    std::shared_ptr<State> state_;
    std::thread worker_;
};

// A cancellation domain also fences queued events from old connections.
class Generation {
public:
    Operation begin(std::chrono::milliseconds timeout = OperationTimeout);
    uint64_t advance();
    bool accepts(uint64_t generation) const;
    void close();
private:
    mutable std::mutex mutex_;
    uint64_t value_ = 1;
    bool closed_ = false;
    std::vector<std::weak_ptr<std::atomic<bool>>> pending_;
};

template<class F>
auto waitFor(SerialExecutor& executor, const Operation& op, F work) -> decltype(work(op)) {
    using Result = decltype(work(op));
    if (executor.isCurrent()) throw Failure(ErrorCodes::SimpleCommKitBleReentrantOperation);
    op.check();
    auto task = std::make_shared<std::packaged_task<Result()>>([op, work = std::move(work)]() mutable {
        op.check();
        if constexpr (std::is_void_v<Result>) { work(op); op.check(); }
        else { auto result = work(op); op.check(); return result; }
    });
    auto future = task->get_future();
    if (!executor.post([task] { (*task)(); })) throw Failure(ErrorCodes::SimpleCommKitBleCancelled);
    // Cancellation must wake a synchronous caller even if the platform is late.
    while (future.wait_until(std::min(op.deadline, Clock::now() + std::chrono::milliseconds(5)))
           != std::future_status::ready) {
        try { op.check(); } catch (...) { op.cancelled->store(true); throw; }
    }
    op.check();
    return future.get();
}
} // namespace SimpleCommKit::Ble
