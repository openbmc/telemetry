#pragma once

#include "interfaces/json_storage.hpp"
#include "interfaces/report.hpp"
#include "interfaces/report_manager.hpp"
#include "types/report_types.hpp"

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

    virtual std::unique_ptr<interfaces::Report> make(
        const std::string& name, const std::string& reportingType,
        bool emitsReadingsSignal, bool logToMetricReportsCollection,
        Milliseconds period, uint64_t appendLimit,
        const std::string& reportUpdates, ReportManager& reportManager,
        JsonStorage& reportStorage,
        std::vector<LabeledMetricParameters> labeledMetricParams) const = 0;
};

} // namespace interfaces
