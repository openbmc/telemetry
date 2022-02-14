#pragma once

#include "interfaces/json_storage.hpp"
#include "interfaces/report.hpp"
#include "interfaces/report_manager.hpp"
#include "types/readings.hpp"
#include "types/report_action.hpp"
#include "types/report_types.hpp"
#include "types/report_updates.hpp"
#include "types/reporting_type.hpp"

#include <boost/asio/spawn.hpp>

#include <chrono>
#include <memory>
#include <optional>

namespace interfaces
{

class ReportFactory
{
  public:
    virtual ~ReportFactory() = default;

    virtual std::vector<LabeledMetricParameters>
        convertMetricParams(boost::asio::yield_context& yield,
                            const ReadingParameters& metricParams) const = 0;

    virtual std::unique_ptr<interfaces::Report>
        make(const std::string& id, const std::string& name,
             const ReportingType reportingType,
             const std::vector<ReportAction>& reportActions,
             Milliseconds period, uint64_t appendLimit,
             const ReportUpdates reportUpdates, ReportManager& reportManager,
             JsonStorage& reportStorage,
             std::vector<LabeledMetricParameters> labeledMetricParams,
             bool enabled, Readings) const = 0;
};

} // namespace interfaces
