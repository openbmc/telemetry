#pragma once

#include "interfaces/report.hpp"

namespace interfaces
{

class ReportManager
{
  public:
    virtual ~ReportManager() = default;

    virtual void removeReport(const interfaces::Report* report) = 0;
    virtual void updateReport(const std::string& name) = 0;
};

} // namespace interfaces
