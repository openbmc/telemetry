#pragma once

#include "core/interfaces/json_storage.hpp"
#include "core/report_name.hpp"
#include "dbus/dbus.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace dbus
{
class ReportConfiguration
{
  public:
    ReportConfiguration(const nlohmann::json& json)
    {
        if (json.at("version") != ReportConfiguration::Version)
        {
            utils::throwError(std::errc::protocol_not_supported,
                              "Persistency protocol version mismatch");
        }

        domain = json.at("domain").get<std::string>();
        name = json.at("name").get<std::string>();
        reportingType = json.at("reportingType").get<std::string>();
        reportAction = json.at("reportAction").get<std::vector<std::string>>();
        scanPeriod = json.at("scanPeriod").get<uint32_t>();
        persistency = json.at("persistency").get<std::string>();

        for (const auto& item : json.at("metricParams"))
        {
            api::SensorPaths sensorsPath;
            for (const auto& path :
                 item.at("sensorPaths").get<std::vector<std::string>>())
            {
                sensorsPath.emplace_back(path);
            }

            metricParams.emplace_back(
                std::move(sensorsPath),
                item.at("operationType")
                    .get<std::tuple_element_t<1, MetricParams>>(),
                item.at("id").get<std::tuple_element_t<2, MetricParams>>(),
                item.at("metricMetadata")
                    .get<std::tuple_element_t<3, MetricParams>>());
        }
    }

    static core::interfaces::JsonStorage::Resource
        path(const core::interfaces::JsonStorage::Resource& directory)
    {
        return directory /
               core::interfaces::JsonStorage::Resource("configuration.json");
    }

    static core::interfaces::JsonStorage::Resource
        path(const core::ReportName& name)
    {
        return path(core::interfaces::JsonStorage::Resource("Report") /
                    name.str());
    }

    static std::vector<core::interfaces::JsonStorage::Resource>
        list(core::interfaces::JsonStorage& storage)
    {
        return storage.list("Report");
    }

    constexpr static int Version = 1;

    std::string domain;
    std::string name;
    std::string reportingType;
    std::vector<std::string> reportAction;
    uint32_t scanPeriod = 0u;
    std::string persistency;
    std::vector<MetricParams> metricParams;
};
} // namespace dbus
