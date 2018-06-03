# Introduction
MAN is a ThreadPool wrote in C++17. The name is chose because, at least in France, it is said than men are not able to do several things at the same times.

# Different classes
## Runnable
### Introduction
The class `Runnable` is defined like that :

    class Runnable<Args...>;

The `Args...` are the types of the arguments to give to the `Runnable::operator();` or its `launch` method.

### Features
* Can handle Functors.
* Can handle Lambdas.
* Can handle function pointers.
* Can retrieve the result if there is any.
* Can retrieve the elapsed time since the beginning of the task.
* Can retrieve the progression if there is one available
* Can retrieve the remaining time if there is a progression.
* Can retrieve the issues if there are some issues

## RunnableQueue
### Introduction
The class `RunnableQueue` is defined as follow :

    RunnableQueue<type_list<ContextsAndArgs...>, type_list<OnlyArgs...>>;

The `ContextsAndArgs...` represents all the arguments that the Runnable will take.

* Contexts mean arguments that lives on the thread on which the Runnable will be run.
* Args mean the argument that you will directly pass to the runnable when you call launch.

## ThreadPool
### Introduction
The classes for manage a thread pool are defined as follow :

    class ThreadPoolWithContextsAndArgs<type_list<Contexts...>, type_list<Args...>>;
    using ThreadPool = ThreadPoolWithContextsAndArgs<type_list<>, type_list<>>;

    template<typename ...Args>
    using ThreadPoolWithArgs = ThreadPoolWithContextsAndArgs<type_list<>, type_list<Args...>>;

    template<typename ...Contexts>
    using ThreadPoolWithContext = ThreadPoolWithContextsAndArgs<type_list<Contexts...>, type_list<>>;

### How to use it ?
The first thing to do is to create a _function_ to run.

    struct Test {
        int operator()(int *a, int b) noexcept {
            auto r = *a + b;
            delete a;
            return r;
        }
    };

Here, the function takes two arguments : `int*, int`. Let's assume that
the first arguments is a context variable, and the second is a direct argument.

The second thing to do is to create a thread pool. We must give to the thread pool the
context types, which is an `int*`, and an argument type, which is an `int` here.
We also need to give a function to initialize each context variables.
After, we call the function `addRunnable` and give it the function and the arguments.
After we wait for all tasks within the threadPool are finished (we could also do it for
the task only) and we check the result.

    man::ThreadPoolWithContextsAndArgs<man::type_list<int*>, man::type_list<int>>
            poolContext{1,
                        []{return new int{42};}};
    auto runnable = poolContext.addRunnable(Test{}, 42);
    poolContext.wait();
    assert(runnable->getResult<int>() == 42 + 42);

# Futures improvements
* Runnable : No dynamic allocation, use aligned_storage instead.
* Runnable : Allow to use only one type to avoid virtual calls.
* Creates a `promise` / `future` types.
* Allows continuation chains : `runnable->then(foo);`
* Make it compatible with C++20 stackless coroutine / or self made coroutines?
