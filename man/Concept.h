#pragma once
#include <string>
#include <typeindex>
#include <optional>
#include "RangeType.h"

namespace man {
enum class KindOfError {
    INFORMATION,
    WARNING,
    ERROR
};
using Issue = std::tuple<std::string, KindOfError>;

template<typename ...Args>
struct Concept {
    Concept(std::type_index returnTypeIndex) : m_returnTypeIndex(returnTypeIndex){}
    virtual ~Concept() noexcept = default;

    virtual void launch(Args...) noexcept = 0;
    virtual void retrieveResult(void *p) noexcept = 0;
    virtual std::optional<Progression> progression() const noexcept = 0;
    virtual std::vector<Issue> issues() const noexcept = 0;

    template<typename T>
    inline void checkReturnType() const noexcept {
        assert(m_returnTypeIndex == typeid(T) && "The return value is not correct");
    }

    std::type_index m_returnTypeIndex;
};
}
