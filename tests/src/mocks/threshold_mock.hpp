#pragma once

#include "interfaces/sensor.hpp"
#include "interfaces/threshold.hpp"
#include "utils/conv_container.hpp"

#include <vector>

#include <gmock/gmock.h>

class ThresholdMock :
    public interfaces::Threshold,
    public std::enable_shared_from_this<ThresholdMock>
{
  public:
    MOCK_METHOD(void, initialize, (), (override));

    MOCK_METHOD(LabeledThresholdParam, getThresholdParam, (),
                (const, override));

    MOCK_METHOD(void, updateSensors, (Sensors newSensors), (override));

    static std::vector<std::shared_ptr<interfaces::Threshold>>
        makeThresholds(const LabeledTriggerThresholdParams& params)
    {
        using namespace testing;
        std::vector<std::shared_ptr<NiceMock<ThresholdMock>>> result;
        if (std::holds_alternative<std::vector<numeric::LabeledThresholdParam>>(
                params))
        {
            auto unpackedParams =
                std::get<std::vector<numeric::LabeledThresholdParam>>(params);
            for (const auto& param : unpackedParams)
            {
                auto& thresholdMock = result.emplace_back(
                    std::make_shared<NiceMock<ThresholdMock>>());
                ON_CALL(*thresholdMock, getThresholdParam())
                    .WillByDefault(Return(param));
            }
        }
        else
        {
            auto unpackedParams =
                std::get<std::vector<discrete::LabeledThresholdParam>>(params);
            for (const auto& param : unpackedParams)
            {
                auto& thresholdMock = result.emplace_back(
                    std::make_shared<NiceMock<ThresholdMock>>());
                ON_CALL(*thresholdMock, getThresholdParam())
                    .WillByDefault(Return(param));
            }
            if (unpackedParams.empty())
            {
                auto& thresholdMock = result.emplace_back(
                    std::make_shared<NiceMock<ThresholdMock>>());
                ON_CALL(*thresholdMock, getThresholdParam())
                    .WillByDefault(Return(std::monostate{}));
            }
        }

        return utils::convContainer<std::shared_ptr<interfaces::Threshold>>(
            result);
    }
};
