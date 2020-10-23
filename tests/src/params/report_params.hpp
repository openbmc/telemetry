#pragma once

#include <chrono>
#include <string>

class ReportParams final
{
  public:
    ReportParams& reportName(std::string val)
    {
        reportNameProperty = std::move(val);
        return *this;
    }

    const std::string& reportName() const
    {
        return reportNameProperty;
    }

    ReportParams& reportingType(std::string val)
    {
        reportingTypeProperty = std::move(val);
        return *this;
    }

    const std::string& reportingType() const
    {
        return reportingTypeProperty;
    }

  protected:
    std::string reportNameProperty = "TestReport";
    std::string reportingTypeProperty = "OnRequest";
};