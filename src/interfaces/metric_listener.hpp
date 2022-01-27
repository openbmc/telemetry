#pragma once

namespace interfaces
{

class MetricListener
{
  public:
    virtual ~MetricListener() = default;

    virtual void metricUpdated() = 0;
};

} // namespace interfaces
