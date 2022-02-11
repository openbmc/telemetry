#pragma once

#include "utils/transform.hpp"

namespace utils
{

template <class R, class Container>
auto convContainer(const Container& container)
{
    return transform(container, [](const auto& item) -> R { return item; });
}

} // namespace utils
