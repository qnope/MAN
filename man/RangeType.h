#pragma once
#include <type_traits>
#include <cassert>
#include "copyable_atomic.h"

struct assert_politic {
    /**
     * When this function is called, it stops the program by calling assert(false)
     */
    void operator()() const noexcept{assert(false);}
};

template<typename Exception>
struct throw_politic {
    /**
     * When this function is called, it throw the exception Exception
     */
    constexpr void operator()() const {throw Exception{};}
};

template<typename T, typename ErrorPolitic>
class RangeType {
public:
    /**
     * Construct a RangeType with min and max as value
     * @param min - The lower value accepted by the range
     * @param max - The higher value accepted by the range
     */
    constexpr RangeType(T min, T max) noexcept : m_min{min}, m_max{max}{}
    constexpr RangeType(const RangeType&) noexcept = default;
    constexpr RangeType(RangeType&&) noexcept = default;
    constexpr RangeType &operator=(const RangeType&) noexcept = default;
    constexpr RangeType &operator=(RangeType&&) noexcept = default;

    constexpr static bool isNoexcept = noexcept(ErrorPolitic{}());

    /**
     * Assign value to m_current and check if the value is correct
     * @param value - The value we want to check
     * @return
     */
    constexpr RangeType &operator =(T value) noexcept(isNoexcept) {
        m_current = std::move(value);
        validate();
        return *this;
    }

    /** This part defines differents operators to manipulate the values **/
#define OP(Op) constexpr RangeType &operator Op(T value) noexcept(isNoexcept) {m_current Op value; validate(); return *this;}
    OP(+=)
    OP(-=)
    OP(/=)
    OP(*=)
#undef OP
#define OP(Op, OpEq)  friend constexpr RangeType operator Op(RangeType a, T b) noexcept(isNoexcept) {RangeType tmp{std::move(a)}; tmp OpEq b; return tmp;}\
                friend constexpr RangeType operator Op(RangeType a, RangeType b) noexcept(isNoexcept) {RangeType tmp{std::move(a)}; tmp OpEq static_cast<T>(b); return tmp;}\
                friend constexpr RangeType operator Op(T a, RangeType b) noexcept(isNoexcept) {RangeType tmp{std::move(b)}; tmp OpEq a; return tmp;}
    OP(+, +=)
    OP(-, -=)
    OP(/, /=)
    OP(*, *=)
#undef OP

    constexpr operator const T() const noexcept {
        return m_current;
    }

private:
    constexpr void validate() const {
        if(m_min > m_current || m_current > m_max) ErrorPolitic{}();
    }

    T m_min;
    T m_current{};
    T m_max;
};

template<typename T, typename ErrorPolitic>
class ProgressionTemplated {
public:
    constexpr static bool isNoexcept = noexcept(ErrorPolitic{}());

    /**
     * Construct a RangeType with min and max as value
     * @param min - The lower value accepted by the range
     * @param max - The higher value accepted by the range
     */
    constexpr ProgressionTemplated(T value) noexcept(isNoexcept) : m_min{0.0}, m_current{value}, m_max{1.0}{
        validate();
    }

    constexpr ProgressionTemplated(const ProgressionTemplated&) noexcept = default;
    constexpr ProgressionTemplated(ProgressionTemplated&&) noexcept = default;
    constexpr ProgressionTemplated &operator=(const ProgressionTemplated&) noexcept = default;
    constexpr ProgressionTemplated &operator=(ProgressionTemplated&&) noexcept = default;

    /**
     * Assign value to m_current and check if the value is correct
     * @param value - The value we want to check
     * @return
     */
    constexpr ProgressionTemplated &operator =(T value) noexcept(isNoexcept) {
        m_current.store(std::move(value), std::memory_order_relaxed);
        validate();
        return *this;
    }

    /** This part defines differents operators to manipulate the values **/
#define OP(Op) constexpr ProgressionTemplated &operator Op(T value) noexcept(isNoexcept) {\
        auto result = m_current.load(std::memory_order_relaxed);\
        result Op value;\
        m_current.store(result, std::memory_order_relaxed);\
        validate();\
        return *this;\
    }
    OP(+=)
    OP(-=)
    OP(/=)
    OP(*=)
#undef OP

    constexpr operator const T() const noexcept {
        return m_current.load(std::memory_order_relaxed);
    }

private:
    constexpr void validate() const {
        auto current = m_current.load(std::memory_order_relaxed);
        if(m_min > current || current > m_max) ErrorPolitic{}();
    }

    T m_min;
    man::copyable_atomic<T> m_current;
    T m_max;
};

using Progression = ProgressionTemplated<float, assert_politic>;
