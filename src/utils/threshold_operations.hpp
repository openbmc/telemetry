#pragma once

#include "interfaces/sensor.hpp"

#include <boost/asio/io_context.hpp>

struct ThresholdOperations
{
    template <typename ThresholdType>
    static void initialize(ThresholdType* thresholdPtr)
    {
        for ([[maybe_unused]] auto& [sensor, detail] :
             thresholdPtr->sensorDetails)
        {
            sensor->registerForUpdates(thresholdPtr->weak_from_this());
        }
        thresholdPtr->initialized = true;
    }

    template <typename ThresholdType>
    static typename ThresholdType::ThresholdDetail& getDetails(
        ThresholdType* thresholdPtr, const interfaces::Sensor& sensor)
    {
        auto it = std::find_if(
            thresholdPtr->sensorDetails.begin(),
            thresholdPtr->sensorDetails.end(),
            [&sensor](const auto& x) { return &sensor == x.first.get(); });
        return *it->second;
    }

    template <typename ThresholdType>
    static void updateSensors(ThresholdType* thresholdPtr, Sensors newSensors)
    {
        typename ThresholdType::SensorDetails newSensorDetails;
        typename ThresholdType::SensorDetails oldSensorDetails =
            thresholdPtr->sensorDetails;

        for (const auto& sensor : newSensors)
        {
            auto it = std::find_if(
                oldSensorDetails.begin(), oldSensorDetails.end(),
                [&sensor](const auto& sd) { return sensor == sd.first; });
            if (it != oldSensorDetails.end())
            {
                newSensorDetails.emplace(*it);
                oldSensorDetails.erase(it);
                continue;
            }

            newSensorDetails.emplace(
                sensor, thresholdPtr->makeDetails(sensor->getName()));
            if (thresholdPtr->initialized)
            {
                sensor->registerForUpdates(thresholdPtr->weak_from_this());
            }
        }

        if (thresholdPtr->initialized)
        {
            for ([[maybe_unused]] auto& [sensor, detail] : oldSensorDetails)
            {
                sensor->unregisterFromUpdates(thresholdPtr->weak_from_this());
            }
        }

        thresholdPtr->sensorDetails = std::move(newSensorDetails);
    }
};
