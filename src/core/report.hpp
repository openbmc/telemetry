#pragma once

#include "core/interfaces/report.hpp"
#include "core/report_name.hpp"
#include "core/sensor.hpp"
#include "log.hpp"

#include <boost/asio/high_resolution_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/core/span.hpp>

#include <chrono>
#include <functional>
#include <memory>
#include <numeric>
#include <vector>

namespace core
{

using namespace std::literals;

class ReportPollerStrategy
{
  public:
    explicit ReportPollerStrategy(
        const ReportName& name,
        const std::vector<std::shared_ptr<interfaces::Metric>>&) :
        name_(name)
    {}

    template <class T>
    void
        execute(const std::vector<std::shared_ptr<interfaces::Metric>>& metrics,
                T&& callback)
    {
        if (tasksLeft_ > 0u)
        {
            LOG_WARNING_T(name_) << "Scanning period too fast";
            return;
        }

        tasksLeft_ = metrics.size();

        for (const auto& metric : metrics)
        {
            metric->async_read(callback);
        }
    }

    bool completeItem()
    {
        return --tasksLeft_ == 0u;
    }

    std::optional<std::chrono::milliseconds>
        getScheduleInterval(interfaces::Report& report) const
    {
        if (report.reportingType() == ReportingType::periodic)
        {
            return report.scanPeriod();
        }
        else
        {
            constexpr auto defaultPollingScanPeriod = 100ms;
            return defaultPollingScanPeriod;
        }
    }

  private:
    const ReportName& name_;
    size_t tasksLeft_ = 0u;
};

class ReportSignalStrategy
{
  public:
    ReportSignalStrategy(
        const ReportName&,
        const std::vector<std::shared_ptr<interfaces::Metric>>& metrics)
    {
        for (const auto& metric : metrics)
        {
            metric->async_read([] {});
            metric->registerForUpdates();
        }
    }

    template <class T>
    void execute(const std::vector<std::shared_ptr<interfaces::Metric>>&,
                 T&& callback)
    {
        callback();
    }

    bool completeItem()
    {
        return true;
    }

    std::optional<std::chrono::milliseconds>
        getScheduleInterval(interfaces::Report& report)
    {
        if (report.reportingType() == ReportingType::periodic)
        {
            return report.scanPeriod();
        }

        return std::nullopt;
    }
};

template <class Strategy>
class Report :
    public interfaces::Report,
    public std::enable_shared_from_this<core::Report<Strategy>>
{
  public:
    Report(boost::asio::io_context& ioc, const ReportName& name,
           std::vector<std::shared_ptr<interfaces::Metric>>&& metrics,
           const ReportingType& reportingType,
           std::vector<ReportAction>&& reportAction,
           std::chrono::milliseconds scanPeriod) :
        ioc_(ioc),
        name_(name), metrics_(std::move(metrics)),
        reportingType_(reportingType), reportAction_(std::move(reportAction)),
        timer_(ioc), scanPeriod_(scanPeriod)
    {
        LOG_DEBUG_T(name_) << "core::Report Constructor";
    }

    Report(const Report&) = delete;
    Report& operator=(const Report&) = delete;

    ~Report()
    {
        LOG_DEBUG_T(name_) << "core::Report ~Report";
    }

    void update() override
    {
        timestamp_ = std::time(0);

        if (readingsUpdated_)
        {
            readingsUpdated_();
        }
    }

    void setCallback(ReadingsUpdated&& callback) override
    {
        readingsUpdated_ = std::move(callback);
    }

    const ReportName& name() const override
    {
        return name_;
    }

    ReportingType reportingType() const override
    {
        return reportingType_;
    }

    const std::vector<ReportAction>& reportAction() const override
    {
        return reportAction_;
    }

    std::time_t timestamp() const override
    {
        return timestamp_;
    }

    std::chrono::milliseconds scanPeriod() const override
    {
        return scanPeriod_;
    }

    boost::beast::span<const std::shared_ptr<interfaces::Metric>>
        metrics() const override
    {
        return {metrics_.data(), metrics_.size()};
    }

    std::vector<Reading> readings() const override
    {
        std::vector<Reading> result;
        result.reserve(metrics_.size());
        std::transform(std::begin(metrics_), std::end(metrics_),
                       std::back_inserter(result), [](const auto& metric) {
                           return Reading{metric, metric->readings()};
                       });
        return result;
    }

    void stop() override
    {
        stopped_ = true;
        timer_.cancel();
    }

    void schedule()
    {
        if (auto interval = strategy_.getScheduleInterval(*this))
        {
            schedule(*interval);
        }
    }

  private:
    static constexpr std::chrono::milliseconds defaultScanPeriod = 100ms;

    std::reference_wrapper<boost::asio::io_context> ioc_;
    const ReportName name_;
    const std::vector<std::shared_ptr<interfaces::Metric>> metrics_;
    const ReportingType reportingType_;
    const std::vector<ReportAction> reportAction_;
    std::time_t timestamp_ = std::time(0);
    ReadingsUpdated readingsUpdated_ = nullptr;

    boost::asio::high_resolution_timer timer_;
    std::chrono::milliseconds scanPeriod_;
    bool stopped_ = false;

    Strategy strategy_{name_, metrics_};

    void schedule(std::chrono::milliseconds interval)
    {
        timer_.expires_from_now(interval);
        timer_.async_wait(std::bind(&Report<Strategy>::tick,
                                    this->weak_from_this(), interval,
                                    std::placeholders::_1));
    }

    static void tick(const std::weak_ptr<Report>& weakSelf,
                     std::chrono::milliseconds interval,
                     const boost::system::error_code& e)
    {
        if (auto self = weakSelf.lock())
        {
            if (self->stopped_)
            {
                return;
            }

            self->strategy_.execute(self->metrics_, [weakSelf] {
                if (auto self = weakSelf.lock())
                {
                    if (self->strategy_.completeItem())
                    {
                        if (self->reportingType_ == ReportingType::periodic &&
                            self->readingsUpdated_)
                        {
                            self->timestamp_ = std::time(0);
                            self->readingsUpdated_();
                        }
                    }
                }
            });

            self->schedule(interval);
        }
    }
};

} // namespace core
