#pragma once
#include <atomic>

namespace man {
template<typename T>
struct copyable_atomic : std::atomic<T> {
    copyable_atomic(T value) noexcept : std::atomic<T>{value}{}
    copyable_atomic(const copyable_atomic &v) noexcept : std::atomic<T>{v.load(std::memory_order_relaxed)}{}
    copyable_atomic(copyable_atomic &&v) noexcept : std::atomic<T>{v.load(std::memory_order_relaxed)}{}

    using std::atomic<T>::operator =;
    copyable_atomic &operator=(copyable_atomic &&v) noexcept {store(v.load(std::memory_order_relaxed), std::memory_order_relaxed);}
    copyable_atomic &operator=(const copyable_atomic &v) noexcept {store(v.load(std::memory_order_relaxed), std::memory_order_relaxed);}
};

}
