#include "BleRuntime.h"
#include <algorithm>
#include <cctype>

namespace SimpleCommKit::Ble {
SerialExecutor::SerialExecutor() : state_(std::make_shared<State>()), worker_([state = state_] {
    for (;;) {
        std::function<void()> work;
        {
            std::unique_lock<std::mutex> lock(state->mutex);
            state->changed.wait(lock, [&] { return state->closed || !state->queue.empty(); });
            if (state->closed) return;
            work = std::move(state->queue.front());
            state->queue.pop_front();
        }
        // Exceptions in application callbacks must not kill the worker/process.
        try { work(); } catch (...) {}
    }
}) {}
SerialExecutor::~SerialExecutor() { finish(); }
void SerialExecutor::finish() {
    close();
    if (worker_.joinable()) {
        if (isCurrent()) worker_.detach();
        else worker_.join();
    }
}
bool SerialExecutor::isCurrent() const { return worker_.get_id() == std::this_thread::get_id(); }
bool SerialExecutor::post(std::function<void()> work) {
    auto state = state_;
    std::lock_guard<std::mutex> lock(state->mutex);
    if (state->closed) return false;
    state->queue.push_back(std::move(work));
    state->changed.notify_one();
    return true;
}
void SerialExecutor::close() {
    auto state = state_;
    // Destroy captures outside the mutex: a capture may itself close a queue.
    std::deque<std::function<void()>> discarded;
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->closed = true;
        discarded.swap(state->queue);
    }
    state->changed.notify_all();
}
Operation Generation::begin(std::chrono::milliseconds timeout) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) throw Failure(ErrorCodes::SimpleCommKitBleCancelled);
    Operation op;
    op.generation = value_;
    op.deadline = Clock::now() + timeout;
    pending_.erase(std::remove_if(pending_.begin(), pending_.end(),
        [](const auto& item) { return item.expired(); }), pending_.end());
    pending_.push_back(op.cancelled);
    return op;
}
uint64_t Generation::advance() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& weak : pending_) if (auto flag = weak.lock()) flag->store(true);
    pending_.clear();
    return ++value_;
}
bool Generation::accepts(uint64_t generation) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !closed_ && value_ == generation;
}
void Generation::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    closed_ = true;
    ++value_;
    for (auto& weak : pending_) if (auto flag = weak.lock()) flag->store(true);
    pending_.clear();
}
std::string canonicalUuid(const std::string& input) {
    std::string uuid = input;
    std::transform(uuid.begin(), uuid.end(), uuid.begin(), [](unsigned char c) { return std::tolower(c); });
    auto hex = [](const std::string& s) {
        return std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isxdigit(c); });
    };
    if ((uuid.size() == 4 || uuid.size() == 8) && hex(uuid)) {
        if (uuid.size() == 4) uuid = "0000" + uuid;
        return uuid + "-0000-1000-8000-00805f9b34fb";
    }
    if (uuid.size() != 36) throw Failure(ErrorCodes::SimpleCommKitBleInvalidArgument);
    for (size_t i = 0; i < uuid.size(); ++i) {
        const bool dash = i == 8 || i == 13 || i == 18 || i == 23;
        if (dash ? uuid[i] != '-' : !std::isxdigit(static_cast<unsigned char>(uuid[i])))
            throw Failure(ErrorCodes::SimpleCommKitBleInvalidArgument);
    }
    return uuid;
}
} // namespace SimpleCommKit::Ble
