#include "../utils/transform_to.hpp"
#include "core/report.hpp"
#include "dbus/report_factory.hpp"
#include "mocks/json_storage.hpp"
#include "mocks/metric.hpp"

#include <chrono>
#include <thread>

#include <gmock/gmock.h>

namespace core
{

using namespace testing;
using namespace std::chrono_literals;

class ReportTest : public Test
{
  public:
    ReportTest()
    {
        metricMocks.emplace_back(makeMetricMock<NiceMock>());
        metricMocks.emplace_back(makeMetricMock<NiceMock>());
    }

    void TearDown() override
    {
        run(10ms);

        for (auto& metric : metricMocks)
        {
            metric->synchronize();
        }

        EXPECT_THAT(runExecuted, Eq(true));
    }

    using Metrics = std::vector<std::shared_ptr<interfaces::Metric>>;
    using ReportActions = std::vector<ReportAction>;

    template <template <typename> class MockType>
    std::shared_ptr<MockType<MetricMock>> makeMetricMock()
    {
        return std::make_shared<MockType<MetricMock>>(ioc);
    }

    void expectsNoMetricsCalled()
    {
        for (auto& metric : metricMocks)
        {
            EXPECT_CALL(*metric, async_read(_)).Times(0);
        }
    }

    Metrics metrics() const
    {
        return utils::transformTo<Metrics::value_type>(metricMocks);
    }

    void run(std::chrono::milliseconds timeout = 100ms)
    {
        if (runExecuted)
        {
            return;
        }

        auto t = std::thread([this, timeout] {
            std::this_thread::sleep_for(timeout);
            ioc.stop();
        });

        ioc.run();
        t.join();

        runExecuted = true;
    }

    boost::asio::io_context ioc;
    std::vector<std::shared_ptr<MetricMock>> metricMocks;

    static constexpr std::chrono::milliseconds pollResolutionRate = 10ms;
    const time_t timestamp = std::time(0);
    bool callbackCalled = false;
    interfaces::Report::ReadingsUpdated callback = [this] {
        callbackCalled = true;
    };
    std::shared_ptr<JsonStorageMock> storageMock =
        std::make_shared<StrictMock<JsonStorageMock>>();
    dbus::ReportFactory<ReportPollerStrategy> reportFactory{
        ioc, pollResolutionRate, storageMock};

  private:
    bool runExecuted = false;
};

class ReportTestWithPeriodicReportingType : public ReportTest
{
  public:
    std::shared_ptr<interfaces::Report> sut()
    {
        if (!sut_)
        {
            sut_ = reportFactory.makeCoreReport(
                ReportName("Domain", "ReportName"), metrics(),
                ReportingType::periodic, ReportActions{}, scanPeriod);
        }
        return sut_;
    }

    std::shared_ptr<interfaces::Report>
        sutWithOneReportWithLongerExecutionTime()
    {
        if (!sut_)
        {
            for (auto& metric : metricMocks)
            {
                EXPECT_CALL(*metric, async_read(_));
            }

            auto metric = makeMetricMock<NiceMock>();
            metric->on_call_async_read(10ms);
            metricMocks.push_back(metric);
        }
        return sut_;
    }

    static constexpr std::chrono::milliseconds scanPeriod = 20ms;

  private:
    std::shared_ptr<interfaces::Report> sut_;
};

TEST_F(ReportTestWithPeriodicReportingType,
       metricsAreNotReadBeforeScanPeriodExpires)
{
    expectsNoMetricsCalled();

    sut();

    run(scanPeriod - 5ms);
}

TEST_F(ReportTestWithPeriodicReportingType,
       metricsAreNotReadWhenReportIsStopped)
{
    expectsNoMetricsCalled();

    sut()->stop();

    run(scanPeriod + 5ms);
}

TEST_F(ReportTestWithPeriodicReportingType,
       metricsAreReadAfterScanPeriodExpires)
{
    for (auto& metric : metricMocks)
    {
        EXPECT_CALL(*metric, async_read(_));
    }

    sut();

    run(scanPeriod + 5ms);
}

TEST_F(ReportTestWithPeriodicReportingType,
       metricsAreReadAfterEachScanPeriodExpiration)
{
    for (auto& metric : metricMocks)
    {
        EXPECT_CALL(*metric, async_read(_)).Times(5);
    }

    sut();

    run(scanPeriod * 5 + 5ms);
}

TEST_F(ReportTestWithPeriodicReportingType,
       metricsReadIsSkippedIfPreviousIsNotFinished)
{
    for (auto& metric : metricMocks)
    {
        metric->on_call_async_read(scanPeriod + 1ms);
        EXPECT_CALL(*metric, async_read(_));
    }

    sut();

    run(scanPeriod * 2 + 5ms);
}

TEST_F(ReportTestWithPeriodicReportingType,
       callbackIsNotCalledBeforeScanPeriodExpires)
{
    sut()->setCallback(std::move(callback));

    run(5ms);

    EXPECT_THAT(callbackCalled, Eq(false));
}

TEST_F(ReportTestWithPeriodicReportingType,
       callbackIsCalledWhenSetAfterScanPeriodExpires)
{
    sut()->setCallback(std::move(callback));

    run(scanPeriod + 5ms);

    EXPECT_THAT(callbackCalled, Eq(true));
}

TEST_F(ReportTestWithPeriodicReportingType,
       callbackIsNotCalledWhenAtLeastOneMetricIsNotRead)
{
    sutWithOneReportWithLongerExecutionTime();
    sut()->setCallback(std::move(callback));

    run(scanPeriod + 5ms);

    EXPECT_THAT(callbackCalled, Eq(false));
}

TEST_F(ReportTestWithPeriodicReportingType,
       callbackIsCalledWhenAllMetricsAreRead)
{
    sutWithOneReportWithLongerExecutionTime();
    sut()->setCallback(std::move(callback));

    run(scanPeriod + 15ms);

    EXPECT_THAT(callbackCalled, Eq(true));
}

struct Params
{
    ReportingType reportingType;
    std::chrono::milliseconds scanPeriod;
};

class WithParams : public ReportTest, public WithParamInterface<Params>
{
  public:
    std::shared_ptr<interfaces::Report> sut()
    {
        if (!sut_)
        {
            sut_ = reportFactory.makeCoreReport(
                ReportName("Domain", "ReportName"), metrics(),
                GetParam().reportingType, ReportActions{},
                GetParam().scanPeriod);
        }
        return sut_;
    }

  private:
    std::shared_ptr<interfaces::Report> sut_;
};

class ReportTestErrorDuringParamsValidation : public WithParams
{};

INSTANTIATE_TEST_SUITE_P(ScanPeriod, ReportTestErrorDuringParamsValidation,
                         Values(Params{ReportingType::periodic, 0ms},
                                Params{ReportingType::periodic, 15ms},
                                Params{ReportingType::onChange, 10ms},
                                Params{ReportingType::onChange, 15ms},
                                Params{ReportingType::onRequest, 10ms},
                                Params{ReportingType::onRequest, 15ms}));

TEST_P(ReportTestErrorDuringParamsValidation,
       throwsExceptionWhenCreatedWithInvalidParams)
{
    EXPECT_ANY_THROW(sut());
}

class ReportTestCommonForNotPeriodicReportingType : public WithParams
{};

TEST_P(ReportTestCommonForNotPeriodicReportingType,
       callbackIsNotCalledAfterScanPeriodExpires)
{
    sut()->setCallback(std::move(callback));

    run(GetParam().scanPeriod + 5ms);

    EXPECT_THAT(callbackCalled, Eq(false));
}

INSTANTIATE_TEST_SUITE_P(OnRequest, ReportTestCommonForNotPeriodicReportingType,
                         Values(Params{ReportingType::onRequest, 0ms}));

INSTANTIATE_TEST_SUITE_P(OnChange, ReportTestCommonForNotPeriodicReportingType,
                         Values(Params{ReportingType::onChange, 0ms}));

struct ReportTestCommonForAllReportingTypes : public WithParams
{};

INSTANTIATE_TEST_SUITE_P(OnRequest, ReportTestCommonForAllReportingTypes,
                         Values(Params{ReportingType::onRequest, 0ms}));

INSTANTIATE_TEST_SUITE_P(OnChange, ReportTestCommonForAllReportingTypes,
                         Values(Params{ReportingType::onChange, 0ms}));

INSTANTIATE_TEST_SUITE_P(Periodic, ReportTestCommonForAllReportingTypes,
                         Values(Params{ReportingType::periodic, 10ms}));

TEST_P(ReportTestCommonForAllReportingTypes, scanPeriodIsAssigned)
{
    EXPECT_THAT(sut()->scanPeriod(), Eq(GetParam().scanPeriod));
}

TEST_P(ReportTestCommonForAllReportingTypes, timestampIsChangedOnUpdate)
{
    const auto timestamp = sut()->timestamp();

    std::this_thread::sleep_for(1s);

    sut()->update();

    EXPECT_THAT(sut()->timestamp(), Gt(timestamp));
}

TEST_P(ReportTestCommonForAllReportingTypes, callbackIsCalledOnUpdate)
{
    sut()->setCallback(std::move(callback));

    sut()->update();

    EXPECT_THAT(callbackCalled, Eq(true));
}

TEST_P(ReportTestCommonForAllReportingTypes,
       noCallbackOnUpdateWhenCallbackIsNotSet)
{
    sut()->update();

    EXPECT_THAT(callbackCalled, Eq(false));
}

TEST_P(ReportTestCommonForAllReportingTypes,
       readingsContainAllReadingDoneByMetrics)
{
    std::vector<std::vector<std::optional<interfaces::Metric::Reading>>>
        readings(metricMocks.size());
    readings[0].emplace_back(interfaces::Metric::Reading{12.0, timestamp});

    std::vector<interfaces::Report::ReadingView> expected;
    for (size_t i = 0; i < metricMocks.size(); ++i)
    {
        expected.emplace_back(interfaces::Report::ReadingView(
            metricMocks[i], {readings[i].data(), readings[i].size()}));
    }

    metricMocks[0]->reading_to_return.emplace_back(
        interfaces::Metric::Reading{12.0, timestamp});

    EXPECT_THAT(sut()->readings(), ElementsAreArray(expected));
}

} // namespace core
