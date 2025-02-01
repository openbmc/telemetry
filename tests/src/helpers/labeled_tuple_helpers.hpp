#pragma once

#include "utils/labeled_tuple.hpp"

#include <iomanip>

namespace utils
{

template <class... Args, class... Labels>
inline void PrintTo(
    const LabeledTuple<std::tuple<Args...>, Labels...>& labeledTuple,
    std::ostream* os)
{
    nlohmann::json json;
    to_json(json, labeledTuple);

    (*os) << std::setw(2) << json;
}

} // namespace utils
