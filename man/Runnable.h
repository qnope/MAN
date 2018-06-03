#pragma once
#include <thread>
#include "Model.h"
#include "Chrono.h"

namespace man {
template<typename ...Args>
class Runnable {
    template<typename ..._Args>
    friend std::optional<Progression> getProgression(const Runnable<_Args...> &runnable) noexcept;

    template<typename ..._Args>
    friend std::vector<Issue> getIssues(const Runnable<_Args...> &runnable) noexcept;

public:
    template<typename T>
    Runnable(T t) noexcept :
        m_objectToRun{std::make_unique<Model<special_decay_t<T>, Args...>>(t)} {}

    Runnable(Runnable &&runnable) noexcept = default;

    Runnable(const Runnable &) = delete;
    Runnable() noexcept = delete;


    /**
     * Function to retrieve the elapsed time of the runnable object since the task was launch
     * @return The time
     */
    template<typename TimeUnit = std::chrono::milliseconds>
    TimeUnit getElapsedTime() noexcept {
        using namespace std::chrono;

        if(!isStarted()) {
            return TimeUnit{0};
        }

        if(isFinished()) {
            // Acquire the values m_start and m_end at the same time
            std::atomic_thread_fence(std::memory_order_acquire);
            return duration_cast<TimeUnit>(m_endTime - m_startTime);
        }

        else {
            // Acquire only the m_startTime
            std::atomic_thread_fence(std::memory_order_acquire);
            return duration_cast<TimeUnit>(Clock::now() - m_startTime);
        }
    }

    /**
     * Function to retrieve the remaining time of the runnable object if it is available
     *
     * If the progression is not available for many reason (not launched, no getProgression function),
     * the optional will be empty
     * @return The number of millisecond until the end of the task
     */
    template<typename TimeUnit = std::chrono::milliseconds>
    std::optional<TimeUnit> getRemainingTime() noexcept {
        using namespace std::chrono;
        auto epsilon = 0.00001;

        if(auto progression = getProgression(*this); progression.has_value()) {
            auto currentTime = getElapsedTime<nanoseconds>();
            auto timeInNanoseconds = (1.0 / (*progression + epsilon)) * currentTime;
            return duration_cast<TimeUnit>((timeInNanoseconds) * (1.0 - *progression));
        }

        return {};
    }

    /**
     * This function executes the function carried by the runnable object
     */
    void launch(Args... args) noexcept {
        assert(!isStarted() && "Runnable must not be run twice");
        assert(m_objectToRun != nullptr);
        m_startTime = Clock::now();
        std::atomic_thread_fence(std::memory_order_release);
        m_isStarted.store(true, std::memory_order_relaxed);

        m_objectToRun->launch(std::forward<Args>(args)...);

        m_endTime = Clock::now();
        std::atomic_thread_fence(std::memory_order_release);
        m_isFinished.store(true, std::memory_order_relaxed);
    }

    void waitUntilFinished() const noexcept {
        while(!isFinished()) {
            std::this_thread::yield();
        }
    }

    /**
     * This function executes the function carried by the runnable object
     */
    void operator()(Args... args) noexcept {
        launch(std::forward(args)...);
    }

    bool isFinished() const noexcept {
        return m_isFinished.load(std::memory_order_relaxed);
    }

    /**
     * This function return the result of the function carried by the runnable
     *
     * This function assert that 'T' is exactly what the function returns
     * This function throws std::invalid_argument if the result is not available
     * @return T - The result of the function
     * @throw std::invalid_argument
     */
    template<typename T>
    T getResult() {
        m_objectToRun->template checkReturnType<T>();

        if(!isFinished()) {
            throw std::runtime_error{"The result is not available"};
        }

        std::atomic_thread_fence(std::memory_order_acquire);

        T result;
        m_objectToRun->retrieveResult(&result);
        return result;
    }

    bool isStarted() const noexcept {
        return m_isStarted.load(std::memory_order_relaxed);
    }

private:
    std::unique_ptr<Concept<Args...>> m_objectToRun;
    Clock::time_point m_startTime;
    copyable_atomic<bool> m_isStarted{false};
    Clock::time_point m_endTime;
    copyable_atomic<bool> m_isFinished{false};
};

/**
 * Function to retrieve the progression of the runnable object
 *
 * The optional is empty if the task carried by the runnable does not have
 * a getProgression function
 * @param runnable
 * @return The progression of the task
 */
template<typename ...Args>
inline std::optional<Progression> getProgression(const Runnable<Args...> &runnable) noexcept  {
    if(!runnable.isStarted()) {
        return 0.0f;
    }

    if(runnable.isFinished()) {
        return 1.0f;
    }

    return runnable.m_objectToRun->progression();
}

/**
 * Function to retrieve all the issues of the runnable object
 * @param runnable - The task from which we want to retrieve all the issues
 * @return All issues.
 */
template<typename ...Args>
inline std::vector<Issue> getIssues(const Runnable<Args...> &runnable) noexcept  {
    return runnable.m_objectToRun->issues();
}

}
