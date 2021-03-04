#include "collection_function.hpp"

#include <cmath>

namespace details
{

class FunctionSingle : public CollectionFunction
{
  public:
    ReadingItem calculate(const std::vector<ReadingItem>& readings,
                          uint64_t) const override
    {
        return readings.back();
    }

    ReadingItem calculateForStartupInterval(std::vector<ReadingItem>& readings,
                                            uint64_t timestamp) const override
    {
        readings.assign({readings.back()});
        return readings.back();
    }
};

class FunctionMinimum : public CollectionFunction
{
  public:
    ReadingItem calculate(const std::vector<ReadingItem>& readings,
                          uint64_t) const override
    {
        return *std::min_element(readings.begin(), readings.end(),
                                 [](const auto& left, const auto& right) {
                                     return left.second < right.second;
                                 });
    }

    ReadingItem calculateForStartupInterval(std::vector<ReadingItem>& readings,
                                            uint64_t timestamp) const override
    {
        readings.assign({ReadingItem(calculate(readings, timestamp))});
        return readings.back();
    }
};

class FunctionMaximum : public CollectionFunction
{
  public:
    ReadingItem calculate(const std::vector<ReadingItem>& readings,
                          uint64_t) const override
    {
        return *std::max_element(readings.begin(), readings.end(),
                                 [](const auto& left, const auto& right) {
                                     return left.second < right.second;
                                 });
    }

    ReadingItem calculateForStartupInterval(std::vector<ReadingItem>& readings,
                                            uint64_t timestamp) const override
    {
        readings.assign({ReadingItem(calculate(readings, timestamp))});
        return readings.back();
    }
};

class FunctionAverage : public CollectionFunction
{
  public:
    ReadingItem calculate(const std::vector<ReadingItem>& readings,
                          uint64_t timestamp) const override
    {
        auto valueSum = 0.;
        auto timeSum = uint64_t{0};
        for (auto it = readings.begin(); it != std::prev(readings.end()); ++it)
        {
            const auto kt = std::next(it);
            const auto duration = kt->first - it->first;
            valueSum += it->second * duration;
            timeSum += duration;
        }

        const auto duration = timestamp - readings.back().first;
        valueSum += readings.back().second * duration;
        timeSum += duration;

        return ReadingItem{timestamp, valueSum / timeSum};
    }

    ReadingItem calculateForStartupInterval(std::vector<ReadingItem>& readings,
                                            uint64_t timestamp) const override
    {
        auto result = calculate(readings, timestamp);
        if (std::isfinite(result.second))
        {
            readings.assign({ReadingItem(readings.front().first, result.second),
                             ReadingItem(timestamp, readings.back().second)});
        }
        return result;
    }
};

class FunctionSummation : public CollectionFunction
{
  public:
    ReadingItem calculate(const std::vector<ReadingItem>& readings,
                          uint64_t timestamp) const override
    {
        auto valueSum = 0.;
        for (auto it = readings.begin(); it != std::prev(readings.end()); ++it)
        {
            const auto kt = std::next(it);
            const auto duration = kt->first - it->first;
            valueSum += it->second * duration;
        }

        const auto duration = timestamp - readings.back().first;
        valueSum += readings.back().second * duration;

        return ReadingItem{timestamp, valueSum};
    }

    ReadingItem calculateForStartupInterval(std::vector<ReadingItem>& readings,
                                            uint64_t timestamp) const override
    {
        auto result = calculate(readings, timestamp);
        if (std::isfinite(result.second) && timestamp > 0u)
        {
            readings.assign({ReadingItem(timestamp - 1u, result.second),
                             ReadingItem(timestamp, readings.back().second)});
        }
        return result;
    }
};

std::shared_ptr<CollectionFunction>
    makeCollectionFunction(OperationType operationType)
{
    using namespace std::string_literals;

    switch (operationType)
    {
        case OperationType::single:
            return std::make_shared<FunctionSingle>();
        case OperationType::min:
            return std::make_shared<FunctionMinimum>();
        case OperationType::max:
            return std::make_shared<FunctionMaximum>();
        case OperationType::avg:
            return std::make_shared<FunctionAverage>();
        case OperationType::sum:
            return std::make_shared<FunctionSummation>();
        default:
            throw std::runtime_error("op: "s +
                                     utils::enumToString(operationType) +
                                     " is not supported"s);
    }
}

} // namespace details
