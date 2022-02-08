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
};

} // namespace interfaces
