#pragma once

#include "core/interfaces/metric.hpp"
#include "core/report_action.hpp"
#include "core/report_name.hpp"
#include "core/reporting_type.hpp"
#include "utils/compare_underlying_objects.hpp"

namespace core::interfaces
{

class Report
{
  public:
    using ReadingsUpdated = std::function<void(void)>;

    struct ReadingView final
    {
        ReadingView(const std::shared_ptr<const interfaces::Metric>& metric_,
                    boost::beast::span<const std::optional<Metric::Reading>>
                        readings_) :
            metric(metric_),
            readings(readings_)
        {}

        std::shared_ptr<const interfaces::Metric> metric;
        boost::beast::span<const std::optional<Metric::Reading>> readings;

        bool operator==(const ReadingView& other) const
        {
            return utils::CompareUnderlyingObjects()(metric, other.metric) &&
                   std::equal(std::begin(readings), std::end(readings),
                              std::begin(other.readings),
                              std::end(other.readings));
        }
    };

    virtual ~Report() = default;

    virtual void update() = 0;
    virtual void setCallback(ReadingsUpdated&& callback) = 0;
    virtual const ReportName& name() const = 0;
    virtual ReportingType reportingType() const = 0;
    virtual const std::vector<ReportAction>& reportAction() const = 0;
    virtual std::time_t timestamp() const = 0;
    virtual std::chrono::milliseconds scanPeriod() const = 0;
    virtual const std::vector<std::shared_ptr<interfaces::Metric>>&
        metrics() const = 0;
    virtual std::vector<ReadingView> readings() const = 0;
    virtual void stop() = 0;
};

} // namespace core::interfaces
