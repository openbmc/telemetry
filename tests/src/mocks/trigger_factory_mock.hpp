#pragma once

#include "interfaces/trigger_factory.hpp"
#include "mocks/trigger_mock.hpp"
#include "params/trigger_params.hpp"
#include "trigger.hpp"
#include "utils/conversion_trigger.hpp"

#include <gmock/gmock.h>

class TriggerFactoryMock : public interfaces::TriggerFactory
{
  public:
    TriggerFactoryMock()
    {
        using namespace testing;

        ON_CALL(*this, make(A<const std::string&>(), _, _, _, _, _, _, _))
            .WillByDefault(WithArgs<0>(Invoke([](const std::string& id) {
                return std::make_unique<NiceMock<TriggerMock>>(id);
            })));
    }

    MOCK_METHOD(std::unique_ptr<interfaces::Trigger>, make,
                (const std::string& id, const std::string& name,
                 const std::vector<std::string>& triggerActions,
                 const std::vector<std::string>& reportIds,
                 interfaces::TriggerManager& triggerManager,
                 interfaces::JsonStorage& triggerStorage,
                 const LabeledTriggerThresholdParams& labeledThresholdParams,
                 const std::vector<LabeledSensorInfo>& labeledSensorsInfo),
                (const, override));

    MOCK_METHOD(std::vector<LabeledSensorInfo>, getLabeledSensorsInfo,
                (boost::asio::yield_context&, const SensorsInfo&),
                (const, override));

    auto& expectMake(
        std::optional<TriggerParams> paramsOpt,
        const testing::Matcher<interfaces::TriggerManager&>& tm,
        const testing::Matcher<interfaces::JsonStorage&>& triggerStorage)
    {
        using namespace testing;

        if (paramsOpt)
        {
            const TriggerParams& params = *paramsOpt;

            const auto sensorInfos =
                utils::fromLabeledSensorsInfo(params.sensors());

            ON_CALL(*this, getLabeledSensorsInfo(_, sensorInfos))
                .WillByDefault(Return(params.sensors()));

            return EXPECT_CALL(
                *this, make(params.id(), params.name(), params.triggerActions(),
                            params.reportIds(), tm, triggerStorage,
                            params.thresholdParams(), params.sensors()));
        }
        else
        {
            const std::vector<LabeledSensorInfo> dummy = {
                {"service99",
                 "/xyz/openbmc_project/sensors/temperature/BMC_Temp99",
                 "metadata99"}};

            ON_CALL(*this, getLabeledSensorsInfo(_, _))
                .WillByDefault(Return(dummy));

            return EXPECT_CALL(*this,
                               make(_, _, _, _, tm, triggerStorage, _, dummy));
        }
    }
};
