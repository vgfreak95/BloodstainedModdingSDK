#pragma once
#include <functional>
#include <mutex>
#include <queue>

class ThreadQueue {
   public:
    static ThreadQueue& Instance() {
        static ThreadQueue instance;
        return instance;
    }

    void Enqueue(std::function<void()> fn) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(fn);
    }

    void Flush() {
        std::lock_guard<std::mutex> lock(mutex);
        while (!queue.empty()) {
            queue.front()();
            queue.pop();
        }
    }

   private:
    std::queue<std::function<void()>> queue;
    std::mutex mutex;
};
