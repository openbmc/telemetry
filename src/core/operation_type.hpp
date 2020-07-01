#pragma once

#include <string>
#include <vector>

namespace core
{

// TODO: DOCS: Weird formatting in docs. Nonexisting types (object, enum)
enum class OperationType
{
    single,
    max,
    min,
    average,
    sum
};

const std::vector<std::pair<OperationType, std::string>>
    operationTypeConvertData = {{OperationType::single, "SINGLE"},
                                {OperationType::max, "MAX"},
                                {OperationType::min, "MIN"},
                                {OperationType::average, "AVERAGE"},
                                {OperationType::sum, "SUM"}};

} // namespace core
