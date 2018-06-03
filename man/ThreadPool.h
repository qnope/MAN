#pragma once
#include <thread>
#include <algorithm>
#include <tuple>
#include "RunnableQueue.h"

namespace man {
template<typename ...>
class ThreadPoolWithContextsAndArgs;

template<typename ... Contexts, typename ... Args>
class ThreadPoolWithContextsAndArgs<type_list<Contexts...>, type_list<Args...>> {
    using Queue = RunnableQueue<type_list<Contexts..., Args...>, type_list<Args...>>;
    using RunnableAndArgs = typename Queue::RunnableAndArgs;
    using Context = std::tuple<Contexts...>;
public:
    /**
     * Construct the thread pool
     * @param numberOfThreads
     * @param function that initialize each Context variable
     */
    template<typename ...Fs>
    ThreadPoolWithContextsAndArgs(std::size_t numberOfThreads, Fs&& ...initializers) noexcept :
        m_threadNumber{numberOfThreads},
        m_queues{numberOfThreads} {
        static_assert(sizeof...(Contexts) == sizeof...(Fs), "Each Context must have an initializer");
        for(std::size_t i{0}; i < numberOfThreads; ++i) {
            m_threads.emplace_back([this, i, &initializers...] {
                run(std::ref(m_queues[i]),
                    std::forward<Fs>(initializers)...);
            });
        }
    }

    /**
     * Construct the thread pool
     */
    ThreadPoolWithContextsAndArgs() noexcept :
        ThreadPoolWithContextsAndArgs{std::max<std::size_t>(std::thread::hardware_concurrency() - 1, 1)}{}

    /**
     * This function add a runnable into the runnables collection.
     *
     * It schedules it to be launch by another thread later
     * @param runnable
     * @return a ptr on this runnable
     */
    template<typename T>
    Runnable<Contexts..., Args...> *addRunnable(T &&runnable, Args... args) {
        RunnableAndArgs *runnablePtr = &m_runnables.emplace_back(
            std::move(runnable),
            std::forward<Args>(args)...
        );

        if(tryToPushToOneQueue(runnablePtr)) {
            return std::addressof(std::get<0>(*runnablePtr));
        }

        m_queues[(m_runnables.size() - 1) % m_threadNumber].push(runnablePtr);

        return std::addressof(std::get<0>(*runnablePtr));
    }

    /**
     * Wait for all runnables to finish
     */
    void wait() noexcept {
        for(auto &runnable : m_runnables) {
            std::get<0>(runnable).waitUntilFinished();
        }
    }

    /**
     * Wait for all runnables to finish and clear the vector of runnable
     */
    void clear() noexcept {
        wait();

        m_runnables.clear();
    }

    ~ThreadPoolWithContextsAndArgs() noexcept {
        auto areFinished = [](auto &runnable){return std::get<0>(runnable).isFinished();};
        assert(std::all_of(m_runnables.begin(), m_runnables.end(), areFinished));

        for(auto &queue : m_queues) {
            queue.finish();
        }

        for(auto &thread : m_threads) {
            thread.join();
        }
    }

private:

    /**
     * Function that runs on another thread
     * @param currentQueue
     * @param initializers
     */
    template<typename ...Fs>
    void run(Queue &currentQueue, Fs&& ...initializers) noexcept {
        Context vars{initializers()...};
        while(!currentQueue.isDone()) {
            if(auto runnable = getRunnableAndArgs(currentQueue); runnable != nullptr) {
                auto applyContext = [runnable](auto &...contexts) {
                    auto applyArgs = [&contexts...](Runnable<Contexts..., Args...> &runnable, auto &&... args) noexcept {
                        runnable.launch(contexts..., std::forward<decltype(args)>(args)...);
                    };
                    std::apply(applyArgs, *runnable);
                };
                std::apply(applyContext, vars);
            }

            else {
                std::this_thread::yield();
            }
        }
    }

    RunnableAndArgs *getRunnableAndArgs(Queue &currentQueue) noexcept {
        if(auto runnable = tryToPopFromOneQueue(); runnable != nullptr) {
            return runnable;
        }

        return currentQueue.pop();
    }

    RunnableAndArgs *tryToPopFromOneQueue() noexcept {
        for(auto &queue : m_queues) {
            if(auto runnable = queue.pop(std::try_to_lock); runnable != nullptr) {
                return runnable;
            }
        }

        return nullptr;
    }

    bool tryToPushToOneQueue(RunnableAndArgs *runnablePtr) noexcept {
        for(auto &queue : m_queues) {
            if(queue.push(runnablePtr, std::try_to_lock)) {
                return true;
            }
        }

        return false;
    }

private:
    const std::size_t m_threadNumber;
    std::vector<std::thread> m_threads;
    std::vector<RunnableAndArgs> m_runnables;
    std::vector<Queue> m_queues;
};

using ThreadPool = ThreadPoolWithContextsAndArgs<type_list<>, type_list<>>;

template<typename ...Args>
using ThreadPoolWithArgs = ThreadPoolWithContextsAndArgs<type_list<>, type_list<Args...>>;

template<typename ...Contexts>
using ThreadPoolWithContext = ThreadPoolWithContextsAndArgs<type_list<Contexts...>, type_list<>>;

}
