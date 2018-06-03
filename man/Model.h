#pragma once
#include "Concept.h"
#include "Trait.h"

namespace man {
template<typename T, typename ...Args>
struct Model : Concept<Args...> {

    /**
     * The nothing type is used because it is not possible to create
     * a variable with type void
     */
    struct nothing{};
    using _ReturnType = decltype(std::declval<T>()(std::declval<Args>()...));
    static constexpr bool isNoReturn = std::is_same_v<_ReturnType, void>;
    using ReturnType = std::conditional_t<isNoReturn, nothing, _ReturnType>;

    /**
     * Those expressions express the possibility to call this functions on
     * the type carried by the Model
     */
    template<typename U>
    using progressionExpression = decltype(getProgression(std::declval<U&>()));
    template<typename U>
    using issuesExpression = decltype(getIssues(std::declval<U&>()));

    static constexpr bool hasProgression = is_valid_v<T, progressionExpression>;
    static constexpr bool hasIssues = is_valid_v<T, issuesExpression>;

    // To know if the function is noexcept or not
    static constexpr bool isNoexcept = noexcept(std::declval<T>()(std::declval<Args>()...));

    static_assert(isNoexcept);

    template<typename _T>
    Model(_T data) :
        Concept<Args...>{typeid(ReturnType)}, m_data(data){}

    void launch(Args... args) noexcept override {
        if constexpr(isNoReturn) {
            m_data(std::forward<Args>(args)...);
        }

        else {
            m_result = m_data(std::forward<Args>(args)...);
        }
    }

    void retrieveResult(void *p) noexcept override {
        auto &value = *static_cast<ReturnType*>(p);
        value = std::move(m_result);
    };

    std::optional<Progression> progression() const noexcept override {
        if constexpr(hasProgression) {
            return getProgression(m_data);
        }

        return {};
    }

    std::vector<Issue> issues() const noexcept override {
        if constexpr(hasIssues) {
            return getIssues(m_data);
        }

        return {};
    }

    T m_data;
    ReturnType m_result;
};
}
