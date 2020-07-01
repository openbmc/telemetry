#pragma once

#include "core/sensor.hpp"
#include "dbus/dbus.hpp"
#include "log.hpp"

#include <sdbusplus/asio/connection.hpp>

#include <functional>
#include <memory>

namespace dbus
{
class Sensor : public core::Sensor
{
    using ValueVariant = std::variant<double, int64_t>;

  public:
    class Id : public core::Sensor::Id
    {
      public:
        Id(const std::string& service, const std::string& path) :
            service_(service), path_(path)
        {
            LOG_DEBUG_T(path_) << "dbus::Sensor::Id Constructor";

            // /xyz/openbmc_project/sensors/<domain>/<name>
            constexpr unsigned ExpectedTokensCount = 6;
            std::vector<boost::iterator_range<std::string::iterator>> tokens;
            boost::split(tokens, path_, boost::is_any_of("/"));

            if (tokens.size() != ExpectedTokensCount)
            {
                LOG_ERROR << "Malformed sensor path: " << path_;
                utils::throwError(std::errc::bad_file_descriptor,
                                  "Malformed sensor path: " + path_);
            }

            domain_ = std::string_view(&tokens[4].front(), tokens[4].size());
            name_ = std::string_view(&tokens[5].front(), tokens[5].size());
        }

        virtual ~Id()
        {
            LOG_DEBUG_T(path_) << "dbus::Sensor::Id ~Id()";
        }

        virtual bool operator==(
            const core::interfaces::Sensor::Id& other) const override final
        {
            return std::make_tuple(type(), str()) ==
                   std::make_tuple(other.type(), other.str());
        }

        virtual bool operator<(
            const core::interfaces::Sensor::Id& other) const override final
        {
            return std::make_tuple(type(), str()) <
                   std::make_tuple(other.type(), other.str());
        }

        virtual std::string_view type() const override
        {
            static const std::string_view id = typeid(decltype(*this)).name();
            return id;
        }

        virtual std::string_view str() const override
        {
            return path_;
        }

        std::string_view path() const
        {
            return path_;
        }

        std::string_view service() const
        {
            return service_;
        }

        std::string_view domain() const
        {
            return domain_;
        }

        std::string_view name() const
        {
            return name_;
        }

      private:
        std::string service_;
        std::string path_;
        std::string_view domain_;
        std::string_view name_;
    };

    Sensor(std::shared_ptr<const dbus::Sensor::Id> id,
           std::shared_ptr<sdbusplus::asio::connection> bus) :
        core::Sensor(id),
        bus_(bus), id_(id)
    {
        LOG_DEBUG_T(id_) << "dbus::Sensor Constructor";
    }

    virtual ~Sensor()
    {
        LOG_DEBUG_T(id_) << "dbus::Sensor ~Sensor";
    }

    bool operator==(const core::interfaces::Sensor& other) const override
    {
        return utils::CompareUnderlyingObjects()(id(), other.id());
    }

    bool operator<(const core::interfaces::Sensor& other) const override
    {
        return utils::CompareLessUnderlyingObjects()(id(), other.id());
    }

    void async_read(core::interfaces::Sensor::ReadCallback&& callback) override
    {
        bus_->async_method_call(
            [id = id_,
             callback = std::move(callback)](const boost::system::error_code& e,
                                             const ValueVariant& valueVariant) {
                if (e)
                {
                    LOG_ERROR_T(id) << "D-Bus 'Get' failed";
                    callback(e, -1);
                    return;
                }

                auto value = readValue(valueVariant);
                if (!value)
                {
                    LOG_ERROR_T(id) << "Unexpected 'Value' type";
                    core::Sensor::readFailedError(callback);
                    return;
                }

                callback(e, *value);
            },
            std::string(id_->service()), std::string(id_->path()),
            "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.Sensor.Value", "Value");
    }

    void registerForUpdates(
        core::interfaces::Sensor::ReadCallback&& callback) override
    {
        if (signalMonitor_)
        {
            return;
        }

        using namespace std::string_literals;

        const auto param = "type='signal',member='PropertiesChanged',path='"s +
                           std::string(id_->path()) + "',"s +
                           "arg0='xyz.openbmc_project.Sensor.Value'"s;

        signalMonitor_ = std::make_unique<sdbusplus::bus::match::match>(
            *bus_, param,
            [name = id_->name(), callback = std::move(callback)](
                sdbusplus::message::message& message) {
                // Callback when dbus signal occurs
                std::string iface;
                boost::container::flat_map<std::string, ValueVariant>
                    changed_properties;
                std::vector<std::string> invalidated_properties;

                message.read(iface, changed_properties, invalidated_properties);

                if (iface == "xyz.openbmc_project.Sensor.Value")
                {
                    const auto it = changed_properties.find("Value");
                    if (it != changed_properties.end())
                    {
                        auto value = readValue(it->second);
                        if (!value)
                        {
                            LOG_ERROR_T(name) << "Unexpected 'Value' type";
                            core::Sensor::readFailedError(callback);
                            return;
                        }

                        callback(boost::system::errc::make_error_code(
                                     boost::system::errc::success),
                                 *value);
                    }
                }
            });
    }

  private:
    std::shared_ptr<sdbusplus::asio::connection> bus_;
    std::shared_ptr<const Id> id_;
    std::unique_ptr<sdbusplus::bus::match::match> signalMonitor_;

    Sensor(const Sensor&) = delete;
    Sensor& operator=(const Sensor&) = delete;

    static std::optional<double> readValue(const ValueVariant& v)
    {
        if (auto d = std::get_if<double>(&v))
        {
            return *d;
        }
        else if (auto i = std::get_if<int64_t>(&v))
        {
            return static_cast<double>(*i);
        }
        else
        {
            return std::nullopt;
        }
    }
}; // namespace dbus

/*
    TODO: Implement 'OnChange' sensors

    sensor::
        onchange::
            monitor each 'PropertyChange', keep time
            notify report each time, reports keeps
                sending updates each PollRateResolution
                if change was present

        periodic::
            monitor each 'PropertyChange', keep time
            report asks periodically of snapshot data
                uses asynchronous api, but value taken
                \ from cache

        onrequest::
            report asks asynchronously, no cache present

          Below is just an example function body for registering for signal
          Proper flow would be the following:
            - report_mgr registers for sensor with 'OnChange'
            - report adds it's own callback to Sensor object
            - sensor object monitors d-bus_ as long as any callback exist
              - TODO: How to detect unregistering ? Direct call? weak_ptr?
*/

} // namespace dbus
