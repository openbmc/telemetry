#pragma once

#include "interfaces/report_manager.hpp"
#include "interfaces/sensor.hpp"
#include "interfaces/threshold.hpp"
#include "interfaces/trigger_factory.hpp"
#include "sensor_cache.hpp"
#include "utils/dbus_mapper.hpp"

#include <sdbusplus/asio/object_server.hpp>

class TriggerFactory : public interfaces::TriggerFactory
{
  public:
    TriggerFactory(std::shared_ptr<sdbusplus::asio::connection> bus,
                   std::shared_ptr<sdbusplus::asio::object_server> objServer,
                   SensorCache& sensorCache);

    std::unique_ptr<interfaces::Trigger>
        make(const std::string& id, const std::string& name,
             const std::vector<std::string>& triggerActions,
             const std::vector<std::string>& reportIds,
             interfaces::TriggerManager& triggerManager,
             interfaces::JsonStorage& triggerStorage,
             const LabeledTriggerThresholdParams& labeledThresholdParams,
             const std::vector<LabeledSensorInfo>& labeledSensorsinfo)
            const override;

    std::vector<LabeledSensorInfo>
        getLabeledSensorsInfo(boost::asio::yield_context& yield,
                              const SensorsInfo& sensorsInfo) const override;
    std::vector<LabeledSensorInfo>
        getLabeledSensorsInfo(const SensorsInfo& sensorsInfo) const override;

    void updateThresholds(
        std::vector<std::shared_ptr<interfaces::Threshold>>& currentThresholds,
        const std::string& triggerId,
        const std::vector<TriggerAction>& triggerActions,
        const std::shared_ptr<std::vector<std::string>>& reportIds,
        const Sensors& sensors,
        const LabeledTriggerThresholdParams& newParams) const override;

    void updateSensors(Sensors& currentSensors,
                       const std::vector<LabeledSensorInfo>& labeledSensorsInfo)
        const override;

  private:
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    SensorCache& sensorCache;

    Sensors getSensors(
        const std::vector<LabeledSensorInfo>& labeledSensorsInfo) const;

    static std::vector<LabeledSensorInfo>
        parseSensorTree(const std::vector<utils::SensorTree>& tree,
                        const SensorsInfo& sensorsInfo);

    void updateDiscreteThresholds(
        std::vector<std::shared_ptr<interfaces::Threshold>>& currentThresholds,
        const std::string& triggerId,
        const std::vector<TriggerAction>& triggerActions,
        const std::shared_ptr<std::vector<std::string>>& reportIds,
        const Sensors& sensors,
        const std::vector<discrete::LabeledThresholdParam>& newParams) const;

    void updateNumericThresholds(
        std::vector<std::shared_ptr<interfaces::Threshold>>& currentThresholds,
        const std::string& triggerId,
        const std::vector<TriggerAction>& triggerActions,
        const std::shared_ptr<std::vector<std::string>>& reportIds,
        const Sensors& sensors,
        const std::vector<numeric::LabeledThresholdParam>& newParams) const;

    void makeDiscreteThreshold(
        std::vector<std::shared_ptr<interfaces::Threshold>>& thresholds,
        const std::string& triggerId,
        const std::vector<TriggerAction>& triggerActions,
        const std::shared_ptr<std::vector<std::string>>& reportIds,
        const Sensors& sensors,
        const discrete::LabeledThresholdParam& thresholdParam) const;

    void makeNumericThreshold(
        std::vector<std::shared_ptr<interfaces::Threshold>>& thresholds,
        const std::string& triggerId,
        const std::vector<TriggerAction>& triggerActions,
        const std::shared_ptr<std::vector<std::string>>& reportIds,
        const Sensors& sensors,
        const numeric::LabeledThresholdParam& thresholdParam) const;

    void makeOnChangeThreshold(
        std::vector<std::shared_ptr<interfaces::Threshold>>& thresholds,
        const std::string& triggerId,
        const std::vector<TriggerAction>& triggerActions,
        const std::shared_ptr<std::vector<std::string>>& reportIds,
        const Sensors& sensors) const;
};
