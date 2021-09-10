#include "metric_definition_manager.hpp"

#include "utils/dbus_mapper.hpp"
#include "utils/transform.hpp"

#include <nlohmann/json.hpp>

#include <concepts>

namespace detail
{

using namespace std::literals::string_view_literals;
using namespace std::literals::string_literals;

template <class T, class U>
bool contains(T&& container, U&& item)
{
    const auto it = std::find(container.begin(), container.end(), item);
    return it != container.end();
}

constexpr auto unitsMapping = std::array{
    std::pair{"xyz.openbmc_project.Sensor.Value.Unit.CFM"sv, "cft_i/min"sv},
    std::pair{"xyz.openbmc_project.Sensor.Value.Unit.Percent"sv, "%"sv},
    std::pair{"xyz.openbmc_project.Sensor.Value.Unit.Watts"sv, "W"sv},
    std::pair{"xyz.openbmc_project.Sensor.Value.Unit.RPMS"sv, "RPM"sv},
    std::pair{"xyz.openbmc_project.Sensor.Value.Unit.DegreesC"sv, "Cel"sv}};

constexpr auto groupToUnitsMapping = std::array{
    std::pair{"voltage"sv, "V"sv},         std::pair{"power"sv, "W"sv},
    std::pair{"current"sv, "A"sv},         std::pair{"fan_tach"sv, "RPM"sv},
    std::pair{"temperature"sv, "Cel"sv},   std::pair{"fan_pwm"sv, "%"sv},
    std::pair{"utilization"sv, "%"sv},     std::pair{"altitude"sv, "m"sv},
    std::pair{"airflow"sv, "cft_i/min"sv}, std::pair{"energy"sv, "J"sv}};

std::optional<std::string> convertUnit(std::string_view dbusUnit,
                                       std::string_view dbusGroup)
{
    const auto it = std::find_if(
        unitsMapping.begin(), unitsMapping.end(),
        [dbusUnit](const auto& item) { return item.first == dbusUnit; });

    if (it != unitsMapping.end())
    {
        return std::string{it->second};
    }

    const auto kt = std::find_if(
        unitsMapping.begin(), unitsMapping.end(), [dbusUnit](const auto& item) {
            if (const auto pos = item.first.find_last_of(".");
                pos != std::string::npos)
            {
                return item.first.substr(pos + 1) == dbusUnit;
            }
            return false;
        });

    if (kt != unitsMapping.end())
    {
        return std::string{kt->second};
    }

    const auto mt = std::find_if(
        groupToUnitsMapping.begin(), groupToUnitsMapping.end(),
        [dbusGroup](const auto& item) { return item.first == dbusGroup; });

    if (mt != groupToUnitsMapping.end())
    {
        return std::string(mt->second);
    }

    return std::nullopt;
}

struct Sensor
{
    Sensor(std::string_view dbusPath, std::string_view dbusService) :
        path(dbusPath), service(dbusService)
    {}

    bool operator==(const Sensor& other) const
    {
        return std::tie(path, service) == std::tie(other.path, other.service);
    }

    std::optional<std::string> unit(
        boost::asio::yield_context& yield,
        const std::shared_ptr<sdbusplus::asio::connection>& connection) const
    {
        if (!unit_)
        {
            auto dbusUnit = getProperty<std::string>(yield, connection, "Unit");
            unit_ = convertUnit(std::move(dbusUnit), parentPathBasename());
        }

        return unit_;
    }

    std::string metricType(
        boost::asio::yield_context& yield,
        const std::shared_ptr<sdbusplus::asio::connection>& connection) const
    {
        return minValue(yield, connection) == maxValue(yield, connection)
                   ? "Numeric"
                   : "Gauge";
    }

    double minValue(
        boost::asio::yield_context& yield,
        const std::shared_ptr<sdbusplus::asio::connection>& connection) const
    {
        if (!minValue_)
        {
            minValue_ =
                getProperty<double, uint64_t, int64_t, uint32_t, int32_t,
                            uint16_t, int16_t>(yield, connection, "MinValue");
        }

        return *minValue_;
    }

    double maxValue(
        boost::asio::yield_context& yield,
        const std::shared_ptr<sdbusplus::asio::connection>& connection) const
    {
        if (!maxValue_)
        {
            maxValue_ =
                getProperty<double, uint64_t, int64_t, uint32_t, int32_t,
                            uint16_t, int16_t>(yield, connection, "MaxValue");
        }

        return *maxValue_;
    }

    std::string parentPathBasename() const
    {
        return sdbusplus::message::object_path{path}.parent_path().filename();
    }

    std::string pathBasename() const
    {
        return sdbusplus::message::object_path{path}.filename();
    }

    std::string path;
    std::string service;

  private:
    template <class T>
    struct ValueVisitor
    {
        template <class U>
        requires(std::convertible_to<U, T>) T operator()(U value) const
        {
            return static_cast<T>(value);
        }

        [[noreturn]] T operator()(auto) const
        {
            throw std::runtime_error("Failed to convert property");
        }
    };

    template <class T, class... Args>
    T getProperty(
        boost::asio::yield_context& yield,
        const std::shared_ptr<sdbusplus::asio::connection>& connection,
        const std::string& property) const
    {
        auto ec = boost::system::error_code{};

        const auto valueVariant =
            connection
                ->yield_method_call<std::variant<std::monostate, T, Args...>>(
                    yield, ec, service, path, "org.freedesktop.DBus.Properties",
                    "Get", "xyz.openbmc_project.Sensor.Value", property);

        if (!ec)
        {
            return std::visit(ValueVisitor<T>(), valueVariant);
        }

        throw std::runtime_error("Failed to get property! "s + property);
    }

    mutable std::optional<std::string> unit_;
    mutable std::optional<double> minValue_;
    mutable std::optional<double> maxValue_;
};

struct Sensors
{
    Sensors(boost::asio::yield_context& yield,
            const std::shared_ptr<sdbusplus::asio::connection>& connection) :
        yield(yield),
        connection(connection)
    {}

    const std::vector<Sensor>& value() const
    {
        if (sensors)
        {
            return *sensors;
        }

        sensors.emplace();

        const auto tree = utils::getSubTreeSensors(yield, connection);
        for (const auto& [sensorPath, serviceToInterfaces] : tree)
        {
            sensors->emplace_back(sensorPath,
                                  getServiceName(serviceToInterfaces));
        }

        return *sensors;
    }

    std::optional<std::reference_wrapper<const Sensor>>
        operator[](std::string_view sensorPath) const
    {
        const auto& data = value();

        const auto it = std::find_if(data.begin(), data.end(),
                                     [sensorPath](const auto& sensor) {
                                         return sensor.path == sensorPath;
                                     });
        if (it == data.end())
        {
            return std::nullopt;
        }
        return *it;
    }

  private:
    static std::string
        getServiceName(const utils::SensorIfaces& serviceToInterfaces)
    {
        for (const auto& [service, interfaces] : serviceToInterfaces)
        {
            return service;
        }

        throw std::runtime_error("Failed to read sensor service name");
    }

    boost::asio::yield_context& yield;
    std::shared_ptr<sdbusplus::asio::connection> connection;
    mutable std::optional<std::vector<Sensor>> sensors;
};

struct Board
{
    Board(boost::asio::yield_context& yield,
          const std::shared_ptr<sdbusplus::asio::connection>& connection,
          const Sensors& sensors, std::string_view path) :
        path(path),
        yield_(yield), connection_(connection), sensors_(sensors)
    {}

    std::string name() const
    {
        return sdbusplus::message::object_path{path}.filename();
    }

    size_t index(const std::vector<std::string>& groups,
                 const Sensor& sensor) const
    {
        const auto& data = sensors();

        auto index = size_t{};

        for (const auto& s : data)
        {
            if (s.path == sensor.path)
            {
                return index;
            }

            if (const auto& group = s.parentPathBasename();
                contains(groups, group))
            {
                ++index;
            }
        }

        throw std::runtime_error("Sensor index not found");
    }

    bool containsSensor(std::string_view sensorDbusPath) const
    {
        const auto& data = sensors();

        const auto it = std::find_if(data.begin(), data.end(),
                                     [sensorDbusPath](const auto& sensor) {
                                         return sensor.path == sensorDbusPath;
                                     });
        return it != data.end();
    }

    const std::vector<Sensor>& sensors() const
    {
        if (sensorsOnBoard_)
        {
            return *sensorsOnBoard_;
        }

        auto ec = boost::system::error_code{};

        const auto sensorsVariant = connection_->yield_method_call<
            std::variant<std::monostate, std::vector<std::string>>>(
            yield_, ec, "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/inventory/system/board/"s + name() +
                "/all_sensors"s,
            "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.Association", "endpoints");

        sensorsOnBoard_ = std::vector<Sensor>{};

        if (std::holds_alternative<std::vector<std::string>>(sensorsVariant))
        {
            for (const auto& path :
                 std::get<std::vector<std::string>>(sensorsVariant))
            {
                if (auto sensor = sensors_[path])
                {
                    sensorsOnBoard_->emplace_back(*sensor);
                }
            }
        }

        std::sort(sensorsOnBoard_->begin(), sensorsOnBoard_->end(),
                  [](const auto& left, const auto& right) {
                      return left.pathBasename() < right.pathBasename();
                  });

        return *sensorsOnBoard_;
    }

    std::string path;

  private:
    boost::asio::yield_context& yield_;
    std::shared_ptr<sdbusplus::asio::connection> connection_;
    const Sensors& sensors_;
    mutable std::optional<std::vector<Sensor>> sensorsOnBoard_;
};

struct Boards
{
    Boards(boost::asio::yield_context& yield,
           const std::shared_ptr<sdbusplus::asio::connection>& connection,
           const Sensors& sensors) :
        yield_(yield),
        connection_(connection), sensors_(sensors)
    {}

    std::optional<std::reference_wrapper<const Board>>
        operator[](const Sensor& sensor) const
    {
        const auto& data = boards();

        const auto it = std::find_if(
            data.begin(), data.end(), [&sensor](const auto& board) {
                return board.containsSensor(sensor.path);
            });

        if (it == data.end())
        {
            return std::nullopt;
        }

        return *it;
    }

  private:
    const std::vector<Board>& boards() const
    {
        if (boards_)
        {
            return *boards_;
        }

        boards_.emplace();

        for (const auto& dbusPath : utils::getChassisList(yield_, connection_))
        {
            boards_->emplace_back(yield_, connection_, sensors_, dbusPath);
        }

        return *boards_;
    }

    boost::asio::yield_context& yield_;
    const std::shared_ptr<sdbusplus::asio::connection>& connection_;
    const Sensors& sensors_;
    mutable std::optional<std::vector<Board>> boards_;
};

constexpr auto mapping = std::array{
    std::pair{"fan_pwm"sv,
              "/redfish/v1/TelemetryService/MetricDefinitions/Fan_Pwm"sv},
    std::pair{"fan_tach"sv,
              "/redfish/v1/TelemetryService/MetricDefinitions/Fan_Tach"sv}};

struct Md
{
    Md(const Sensor& sensor) : sensor(sensor)
    {}

    std::string metricDefinitionName() const
    {
        return sdbusplus::message::object_path{
            std::string{metricDefinitionPath()}}
            .filename();
    }

    std::optional<std::string> metricProperty(const Boards& boards) const
    {
        if (metricProperty_)
        {
            return metricProperty_;
        }

        if (auto b = boards[sensor])
        {
            const Board& board = *b;

            if (const auto& group = sensor.parentPathBasename();
                group == "fan_pwm" || group == "fan_tach")
            {
                metricProperty_ = "/redfish/v1/Chassis/"s + board.name() +
                                  "/Thermal#/Fans/"s +
                                  std::to_string(board.index(
                                      {"fan_pwm", "fan_tach"}, sensor)) +
                                  "/Reading"s;
            }
            else if (group == "temperature")
            {
                metricProperty_ =
                    "/redfish/v1/Chassis/"s + board.name() +
                    "/Thermal#/Temperatures/"s +
                    std::to_string(board.index({"temperature"}, sensor)) +
                    "/ReadingCelsius"s;
            }
            else
            {
                metricProperty_ = "/redfish/v1/Chassis/"s + board.name() +
                                  "/Sensors/"s + sensor.pathBasename() +
                                  "/Reading"s;
            }
        }

        return metricProperty_;
    }

    std::string_view metricDefinitionPath() const
    {
        if (metricDefinitionPath_)
        {
            return *metricDefinitionPath_;
        }

        const auto it = std::find_if(
            mapping.begin(), mapping.end(), [this](const auto& item) {
                return item.first == sensor.parentPathBasename();
            });

        if (it != mapping.end())
        {
            metricDefinitionPath_ = it->second;
        }
        else
        {
            metricDefinitionPath_ =
                "/redfish/v1/TelemetryService/MetricDefinitions/"s +
                sensor.pathBasename();
        }

        return *metricDefinitionPath_;
    }

    Sensor sensor;

  private:
    mutable std::optional<std::string> metricDefinitionPath_;
    mutable std::optional<std::string> metricProperty_;
    mutable std::optional<std::string> unit_;
};

} // namespace detail

class MetricDefinitionManager::MetricDefinitionCollection
{
  public:
    MetricDefinitionCollection() = default;
    MetricDefinitionCollection(const MetricDefinitionCollection&) = delete;
    MetricDefinitionCollection(MetricDefinitionCollection&&) = delete;

    MetricDefinitionCollection&
        operator=(const MetricDefinitionCollection&) = delete;
    MetricDefinitionCollection&
        operator=(MetricDefinitionCollection&&) = delete;

    const std::vector<detail::Md>&
        value(boost::asio::yield_context& yield,
              const std::shared_ptr<sdbusplus::asio::connection>& connection,
              const detail::Sensors& sensors, const detail::Boards& boards)
    {
        for (const auto& sensor : sensors.value())
        {
            const auto it = std::find_if(
                metricDefinitions.begin(), metricDefinitions.end(),
                [&sensor](const auto& md) { return md.sensor == sensor; });

            if (it == metricDefinitions.end())
            {
                if (auto md = detail::Md{sensor}; md.metricProperty(boards))
                {
                    metricDefinitions.emplace_back(std::move(md));
                }
            }
        }

        metricDefinitions.erase(
            std::remove_if(metricDefinitions.begin(), metricDefinitions.end(),
                           [this, &sensors](const auto& md) {
                               const auto it =
                                   std::find_if(sensors.value().begin(),
                                                sensors.value().end(),
                                                [&md](const auto& sensor) {
                                                    return md.sensor == sensor;
                                                });
                               return it == sensors.value().end();
                           }),
            metricDefinitions.end());

        return metricDefinitions;
    }

  private:
    std::vector<detail::Md> metricDefinitions;
};

MetricDefinitionManager::MetricDefinitionManager(
    const std::shared_ptr<sdbusplus::asio::connection>& connectionIn,
    const std::shared_ptr<sdbusplus::asio::object_server>& objServerIn) :
    connection(connectionIn),
    objServer(objServerIn),
    metricDefinitions(std::make_unique<MetricDefinitionCollection>())
{
    metricManagerInterface = objServer->add_unique_interface(
        metricDefinitionManagerPath, metricManagerIfaceName,
        [this](auto& dbusIface) {
            dbusIface.register_method(
                "MapSensorPathToMetricProperty",
                [this](boost::asio::yield_context& yield,
                       const std::string& path) -> std::string {
                    detail::Sensors sensors{yield, connection};
                    detail::Boards boards{yield, connection, sensors};

                    const auto& mds = metricDefinitions->value(
                        yield, connection, sensors, boards);

                    const auto it = std::find_if(
                        mds.begin(), mds.end(), [&path](const auto& md) {
                            return md.sensor.path == path;
                        });

                    if (it == mds.end())
                    {
                        return "";
                    }

                    return it->metricProperty(boards).value_or("");
                });

            dbusIface.register_method(
                "GetMetricDefinition", [this](boost::asio::yield_context& yield,
                                              const std::string& name) {
                    detail::Sensors sensors{yield, connection};
                    detail::Boards boards{yield, connection, sensors};

                    const auto& mds = metricDefinitions->value(
                        yield, connection, sensors, boards);

                    const auto it = std::find_if(
                        mds.begin(), mds.end(), [&name](const auto& md) {
                            return md.metricDefinitionName() == name;
                        });

                    if (it == mds.end())
                    {
                        return nlohmann::json::object().dump();
                    }

                    std::set<std::string> units;
                    std::vector<std::string> metricProperties;

                    for (const auto& md : mds)
                    {
                        if (md.metricDefinitionName() == name)
                        {
                            if (auto metricProperty = md.metricProperty(boards))
                            {
                                metricProperties.push_back(*metricProperty);
                                if (auto unit =
                                        md.sensor.unit(yield, connection))
                                {
                                    units.insert(*unit);
                                }
                            }
                        }
                    }

                    if (units.size() > 1)
                    {
                        return nlohmann::json::object().dump();
                    }

                    auto result = nlohmann::json::object();

                    result["Id"] = it->metricDefinitionName();
                    result["IsLinear"] = true;
                    result["MetricDataType"] = "Decimal";
                    result["MinReadingRange"] =
                        it->sensor.minValue(yield, connection);
                    result["MaxReadingRange"] =
                        it->sensor.maxValue(yield, connection);
                    result["MetricType"] =
                        it->sensor.metricType(yield, connection);
                    result["Name"] = it->metricDefinitionName();
                    result["@odata.id"] = it->metricDefinitionPath();
                    result["@odata.type"] =
                        "#MetricDefinition.v1_0_3.MetricDefinition";
                    result["Units"] = units.size() > 0 ? *units.begin() : "";
                    result["MetricProperties"] = std::move(metricProperties);

                    return result.dump();
                });

            dbusIface.register_method(
                "GetMetricDefinitions",
                [this](boost::asio::yield_context& yield) {
                    detail::Sensors sensors{yield, connection};
                    detail::Boards boards{yield, connection, sensors};

                    auto result = nlohmann::json::object();

                    result["@odata.id"] =
                        "/redfish/v1/TelemetryService/MetricDefinitions";
                    result["@odata.type"] = "#MetricDefinitionCollection."
                                            "MetricDefinitionCollection";
                    result["Name"] = "Metric Definition Collection";
                    result["Members"] = nlohmann::json::array();

                    std::set<std::string_view> members;

                    for (const auto& md : metricDefinitions->value(
                             yield, connection, sensors, boards))
                    {
                        members.insert(md.metricDefinitionPath());
                    }

                    for (const auto& member : members)
                    {
                        result["Members"].push_back({{"@odata.id", member}});
                    }
                    result["Members@odata.count"] = result["Members"].size();

                    return result.dump();
                });
        });
}

MetricDefinitionManager::~MetricDefinitionManager() = default;
