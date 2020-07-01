#pragma once

#include "core/interfaces/report.hpp"
#include "utils/utils.hpp"

#include <chrono>
#include <list>
#include <memory>

using namespace std::literals;

namespace core
{

class ReportManager
{
  public:
    using ReportHandle = std::shared_ptr<interfaces::Report>;
    using ReadingsUpdated = typename interfaces::Report::ReadingsUpdated;

    ReportManager(uint32_t maxReports,
                  std::chrono::milliseconds pollRateResolution) :
        maxReports_(maxReports),
        pollRateResolution_(pollRateResolution)
    {}
    ReportManager(const ReportManager&) = delete;
    ReportManager& operator=(const ReportManager&) = delete;

    ReportHandle add(ReportHandle&& report)
    {
        reports_.push_back(std::move(report));
        auto& handle = reports_.back();

        return handle;
    }

    void remove(ReportHandle& report)
    {
        auto it = std::find(reports_.begin(), reports_.end(), report);
        if (it != reports_.end())
        {
            LOG_DEBUG << "core::ReportManager::remove " << report->name();
            report->stop();
            reports_.erase(it);
        }
    }

    unsigned maxReports() const
    {
        return maxReports_;
    }

    std::chrono::milliseconds pollRateResolution() const
    {
        return pollRateResolution_;
    }

    void checkIfAllowedToAdd(const ReportName& name)
    {
        checkIfMaxReportReached();
        checkIfUnique(name);
    }

  private:
    std::list<ReportHandle> reports_;

    const unsigned maxReports_;
    const std::chrono::milliseconds pollRateResolution_;

    void checkIfUnique(const ReportName& name)
    {
        auto it = std::find_if(reports_.begin(), reports_.end(),
                               [name](const ReportHandle& existing) {
                                   return existing->name() == name;
                               });
        if (it != reports_.end())
        {
            utils::throwError(std::errc::file_exists,
                              "Duplicate report name. Skipping");
        }
    }

    void checkIfMaxReportReached()
    {
        if (reports_.size() >= maxReports_)
        {
            utils::throwError(std::errc::too_many_files_open,
                              "Report creation failed: Report limit reached");
        }
    }
};

} // namespace core
