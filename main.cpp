#include <iostream>
#include "man/ThreadPool.h"

man::ThreadPool pool{};

namespace testReturn {
int return42() noexcept {
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    return 42;
}

void test() {
    auto runnable = pool.addRunnable(return42);

    assert(runnable->isFinished() == false);
    pool.wait();
    assert(runnable->isFinished() == true);
    assert(runnable->getResult<int>() == 42);
}
}

namespace testProgression {
struct Test {
    void operator()() noexcept {
        m_start = Clock::now();
        std::atomic_thread_fence(std::memory_order_release);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    Clock::time_point m_start;
};

Progression getProgression(const Test &test) {
    using namespace std::chrono;
    std::atomic_thread_fence(std::memory_order_acquire);
    auto now = Clock::now();
    auto milli = duration_cast<milliseconds>(now - test.m_start).count();

    return Progression{std::min(1.0f, (float)milli / 2000.0f)};
}

void test() {
    auto runnable = pool.addRunnable(Test{});

    while(!runnable->isFinished()) {
        auto progression = getProgression(*runnable);
        std::cout << *progression << "%" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
}

namespace testRemaining {
void test() {
    using namespace std::chrono;
    auto runnable = pool.addRunnable(testProgression::Test{});

    while(!runnable->isFinished()) {
        auto remainingTime = runnable->getRemainingTime<milliseconds>();
        if(runnable->isStarted()) {
            std::cout << "Remaining time: " << remainingTime.value_or(milliseconds{0}).count() << "ms" << std::endl;
        }
        else {
            std::cout << "Task not launched yet" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
}

namespace testIssue {
struct Test {
    void operator()() noexcept {}
};

std::vector<man::Issue> getIssues(const Test &) {
    return {man::Issue{"Nothing", man::KindOfError::INFORMATION}};
}

void test() {
    auto runnable = pool.addRunnable(Test{});
    pool.wait();
    auto [message, code] = getIssues(*runnable)[0];
    assert(message == "Nothing");
    assert(code == man::KindOfError::INFORMATION);
}
}

namespace testArgs {
struct Test {
    int operator()(int a, int b) noexcept {
        return a + b;
    }
};

void test() {
    man::ThreadPoolWithArgs<int, int> poolDoubleInt{1};
    auto runnable = poolDoubleInt.addRunnable(Test{}, 42, 42);
    poolDoubleInt.wait();
    assert(runnable->getResult<int>() == 42 * 2);
}
}

namespace testContext {
struct Test {
    int operator()(int *a, int b) noexcept {
        auto r = *a + b;
        delete a;
        return r;
    }
};

void test() {
    man::ThreadPoolWithArgsAndContext<man::type_list<int>, man::type_list<int*>>
            poolContext{1,
                        []{return new int{42};}};
    auto runnable = poolContext.addRunnable(Test{}, 42);
    poolContext.wait();
    assert(runnable->getResult<int>() == 42 + 42);
}
}

int main() {
    std::cout << "==TEST RETURN VALUE==" << std::endl;
    testReturn::test();
    std::cout << "==TEST RETURN VALUE OK==\n==TEST PROGRESSION==" << std::endl;
    testProgression::test();
    std::cout << "==TEST PROGRESSION OK==\n==TEST REMAINING==" << std::endl;
    testRemaining::test();
    std::cout << "==TEST REMAINING OK==\n==TEST ISSUE==" << std::endl;
    testIssue::test();
    std::cout << "==TEST ISSUE OK==\n==TEST ARGS==" << std::endl;
    testArgs::test();
    std::cout << "==TEST ARGS OK==\n==TEST CONTEXT==" << std::endl;
    testContext::test();
    std::cout << "==TEST CONTEXT OK==" << std::endl;
    return 0;
}
