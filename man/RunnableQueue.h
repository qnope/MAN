#pragma once
#include <vector>
#include <mutex>
#include <condition_variable>
#include <tuple>
#include "Runnable.h"

namespace man {
template<typename...>
struct type_list{};

template<typename...>
class RunnableQueue;

template<typename... OnlyArgs, typename... ArgsAndContexts>
class RunnableQueue<type_list<ArgsAndContexts...>, type_list<OnlyArgs...>> {
public:
    using RunnableAndArgs = std::tuple<Runnable<ArgsAndContexts...>, OnlyArgs...>;

    RunnableQueue() noexcept {}

    bool isDone() const noexcept {
        return m_done;
    }

    template<typename ...try_to_lock>
    RunnableAndArgs *pop(try_to_lock ...tryToLock) noexcept {
        std::unique_lock lock{m_mutex, tryToLock...};

        if(!lock) {
            return nullptr;
        }

        m_conditionVariable.wait(lock, [this] {
            return m_done || !m_runnables.empty();
        });

        if(m_done || m_runnables.empty()) {
            return nullptr;
        }

        auto runnable = m_runnables.back();
        m_runnables.pop_back();
        return runnable;
    }

    template<typename ...try_to_lock>
    bool push(RunnableAndArgs *runnableToPush, try_to_lock ...tryToLock) noexcept {
        {
            std::unique_lock lock{m_mutex, tryToLock...};

            if(lock) {
                m_runnables.emplace_back(runnableToPush);
            }

            else {
                return false;
            }
        }
        m_conditionVariable.notify_one();
        return true;
    }

    void finish() noexcept {
        {
            std::scoped_lock lock{m_mutex};
            m_done = true;
        }
        m_conditionVariable.notify_all();
    }

    ~RunnableQueue() noexcept {
        assert(m_runnables.empty());
        assert(m_done && "If finish is not called, you have the risk"
                         " to destroy the mutex even if you are using it");
    }

private:
    std::vector<RunnableAndArgs*> m_runnables;
    std::mutex m_mutex;
    std::condition_variable m_conditionVariable;
    bool m_done{false};
};
}
