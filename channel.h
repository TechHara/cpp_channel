#pragma once

#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <optional>

template<typename T>
class Channel;

template<typename T>
std::pair<Channel<T>, Channel<T>> make_channel() {
    auto ptr = std::make_shared<typename Channel<T>::State>();
    Channel<T> sender{ptr, true};
    Channel<T> receiver{std::move(ptr), false};
    return {std::move(sender), std::move(receiver)};
}

template<typename T>
class Channel {
public:
    Channel(Channel const&) = delete;
    Channel(Channel&&) = default;
    Channel& operator=(Channel const&) = delete;
    Channel& operator=(Channel&&) = default;

    bool send(T item) {
        do {
            if (!is_sender || !state) break;
            std::lock_guard lock{state->lock};
            if (state->closed) break;
            state->queue.push(std::move(item));
            state->cv.notify_one();
            return true;
        } while (false);
        return false;
    }

    std::optional<T> recv() {
        do {
            if (is_sender || !state) break;
            std::unique_lock lock{state->lock};
            state->cv.wait(lock, [this]{
                return !state->queue.empty() || state->closed;
            });
            if (state->queue.empty()) break;
            auto x = std::move(state->queue.front());
            state->queue.pop();
            return x;
        } while (false);
        return std::nullopt;
    }

    bool close() {
        do {
            if (!state) break;
            std::lock_guard lock{state->lock};
            if (state->closed) break;
            state->closed = true;
            state->cv.notify_one();
            return true;
        } while (false);
        return false;
    }

    ~Channel() {
        close();
    }

private:
    struct State {
        std::queue<T> queue;
        std::mutex lock;
        std::condition_variable cv;
        bool closed = false;
    };

    std::shared_ptr<State> state;
    bool is_sender;

    explicit Channel(std::shared_ptr<State> state, bool is_sender)
    : state{std::move(state)}, is_sender{is_sender} {}

    friend std::pair<Channel<T>, Channel<T>> make_channel<T>();
};