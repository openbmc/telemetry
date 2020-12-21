#pragma once

#include "interfaces/trigger_factory.hpp"
#include "mocks/trigger_mock.hpp"
#include "params/trigger_params.hpp"

#include <gmock/gmock.h>

class TriggerFactoryMock : public interfaces::TriggerFactory
{
  public:
    TriggerFactoryMock()
    {
        using namespace testing;

        ON_CALL(*this, make(_, _, _, _, _, _, _, _, _))
            .WillByDefault(WithArgs<0>(Invoke([](const std::string& name) {
                return std::make_unique<NiceMock<TriggerMock>>(name);
            })));
    }

    MOCK_METHOD(std::unique_ptr<interfaces::Trigger>, make,
                (const std::string& name, bool isDiscrete, bool logToJournal,
                 bool logToRedfish, bool updateReport,
                 (const std::vector<std::pair<sdbusplus::message::object_path,
                                              std::string>>& sensors),
                 const std::vector<std::string>& reportNames,
                 const TriggerThresholdParams& thresholds,
                 interfaces::TriggerManager& triggerManager),
                (const, override));

    auto& expectMake(
        std::optional<std::reference_wrapper<const TriggerParams>> paramsOpt,
        const testing::Matcher<interfaces::TriggerManager&>& tm)
    {
        if (paramsOpt)
        {
            const TriggerParams& params = *paramsOpt;
            return EXPECT_CALL(
                *this, make(params.name(), params.isDiscrete(),
                            params.logToJournal(), params.logToRedfish(),
                            params.updateReport(), params.sensors(),
                            params.reportNames(), params.thresholds(), tm));
        }
        else
        {
            using testing::_;
            return EXPECT_CALL(*this, make(_, _, _, _, _, _, _, _, tm));
        }
    }
};
