#pragma once
#include <type_traits>

namespace man {
template <class T>
struct unwrap_refwrapper
{
    using type = T;
};

template <class T>
struct unwrap_refwrapper<std::reference_wrapper<T>>
{
    using type = T&;
};

template <class T>
using special_decay_t = typename unwrap_refwrapper<std::decay_t<T>>::type;

template<typename T, template<typename> typename expr, typename = void>
struct is_valid : std::false_type{};

template<typename T, template<typename> typename expr>
struct is_valid<T, expr, std::void_t<expr<T>>> : std::true_type{};

template<typename T, template<typename> typename expr>
inline constexpr bool is_valid_v = is_valid<T, expr>{};
}

