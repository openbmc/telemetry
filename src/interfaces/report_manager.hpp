#pragma once

#include "interfaces/report.hpp"
#include "types/report_types.hpp"

namespace interfaces
{

class ReportManager
{
  public:
    virtual ~ReportManager() = default;

    virtual void removeReport(const interfaces::Report* report) = 0;
    virtual void updateReport(const std::string& id) = 0;
    virtual void updateTriggerIds(const std::string& reportId,
                                  const std::string& triggerId,
                                  TriggerIdUpdate updateType) = 0;
};

} // namespace interfaces
