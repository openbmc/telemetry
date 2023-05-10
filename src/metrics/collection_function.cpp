#include "metrics/collection_function.hpp"

#include <cmath>

namespace metrics
{

class FunctionMinimum : public CollectionFunction
{
  public:
    double calculate(const std::vector<ReadingItem>& readings,
                     Milliseconds) const override
    {
        return std::min_element(
                   readings.begin(), readings.end(),
                   [](const auto& left, const auto& right) {
            return std::make_tuple(!std::isfinite(left.second), left.second) <
                   std::make_tuple(!std::isfinite(right.second), right.second);
                   })
            ->second;
    }

    double calculateForStartupInterval(std::vector<ReadingItem>& readings,
                                       Milliseconds timestamp) const override
    {
        readings.assign(
            {ReadingItem(timestamp, calculate(readings, timestamp))});
        return readings.back().second;
    }
};

class FunctionMaximum : public CollectionFunction
{
  public:
    double calculate(const std::vector<ReadingItem>& readings,
                     Milliseconds) const override
    {
        return std::max_element(
                   readings.begin(), readings.end(),
                   [](const auto& left, const auto& right) {
            return std::make_tuple(std::isfinite(left.second), left.second) <
                   std::make_tuple(std::isfinite(right.second), right.second);
                   })
            ->second;
    }

    double calculateForStartupInterval(std::vector<ReadingItem>& readings,
                                       Milliseconds timestamp) const override
    {
        readings.assign(
            {ReadingItem(timestamp, calculate(readings, timestamp))});
        return readings.back().second;
    }
};

class FunctionAverage : public CollectionFunction
{
  public:
    double calculate(const std::vector<ReadingItem>& readings,
                     Milliseconds timestamp) const override
    {
        auto valueSum = 0.0;
        auto timeSum = Milliseconds{0};
        for (auto it = readings.begin(); it != std::prev(readings.end()); ++it)
        {
            if (std::isfinite(it->second))
            {
                const auto kt = std::next(it);
                const auto duration = kt->first - it->first;
                valueSum += it->second * duration.count();
                timeSum += duration;
            }
        }

        const auto duration = timestamp - readings.back().first;
        valueSum += readings.back().second * duration.count();
        timeSum += duration;

        return valueSum / std::max(timeSum.count(), uint64_t{1u});
    }

    double calculateForStartupInterval(std::vector<ReadingItem>& readings,
                                       Milliseconds timestamp) const override
    {
        auto result = calculate(readings, timestamp);
        if (std::isfinite(result))
        {
            readings.assign({ReadingItem(readings.front().first, result),
                             ReadingItem(timestamp, readings.back().second)});
        }
        return result;
    }
};

class FunctionSummation : public CollectionFunction
{
    using Multiplier = std::chrono::duration<double>;

  public:
    double calculate(const std::vector<ReadingItem>& readings,
                     const Milliseconds timestamp) const override
    {
        auto valueSum = 0.0;
        for (auto it = readings.begin(); it != std::prev(readings.end()); ++it)
        {
            if (std::isfinite(it->second))
            {
                const auto kt = std::next(it);
                const auto multiplier =
                    calculateMultiplier(kt->first - it->first);
                valueSum += it->second * multiplier.count();
            }
        }

        const auto multiplier =
            calculateMultiplier(timestamp - readings.back().first);
        valueSum += readings.back().second * multiplier.count();

        return valueSum;
    }

    double
        calculateForStartupInterval(std::vector<ReadingItem>& readings,
                                    const Milliseconds timestamp) const override
    {
        const auto result = calculate(readings, timestamp);
        if (readings.size() > 2 && std::isfinite(result))
        {
            const auto multiplier =
                calculateMultiplier(timestamp - readings.front().first).count();
            if (multiplier > 0.)
            {
                const auto prevValue = result / multiplier;
                readings.assign(
                    {ReadingItem(readings.front().first, prevValue),
                     ReadingItem(timestamp, readings.back().second)});
            }
        }
        return result;
    }

  private:
    static constexpr Multiplier calculateMultiplier(Milliseconds duration)
    {
        constexpr auto m = Multiplier{Seconds{1}};
        return Multiplier{duration / m};
    }
};

std::shared_ptr<CollectionFunction>
    makeCollectionFunction(OperationType operationType)
{
    using namespace std::string_literals;

    switch (operationType)
    {
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

} // namespace metrics
