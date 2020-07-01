#pragma once

#include <boost/system/error_code.hpp>

#include <memory>
#include <string>
#include <string_view>

namespace core::interfaces
{
class Sensor
{
  public:
    using ReadCallback = std::function<void(const boost::system::error_code& e,
                                            const double value)>;

    class GlobalId final
    {
      public:
        GlobalId(std::string_view type, std::string_view name) :
            type_(type), name_(name)
        {}

        bool operator==(const GlobalId& other) const
        {
            return std::tie(type_, name_) == std::tie(other.type_, other.name_);
        }

        bool operator<(const GlobalId& other) const
        {
            return std::tie(type_, name_) < std::tie(other.type_, other.name_);
        }

        std::string_view type() const
        {
            return type_;
        }

        std::string_view name() const
        {
            return name_;
        }

      private:
        // Should be unique among various overloads of core::interfaces::Sensor
        std::string type_;

        // Should be unique among sensors of the same class
        std::string name_;
    };

    virtual ~Sensor() = default;

    virtual bool operator==(const Sensor& other) const = 0;
    virtual bool operator<(const Sensor& other) const = 0;

    virtual GlobalId globalId() const = 0;
    virtual std::string_view path() const = 0;
    virtual void async_read(interfaces::Sensor::ReadCallback&& callback) = 0;
    virtual void
        registerForUpdates(interfaces::Sensor::ReadCallback&& callback) = 0;
};

} // namespace core::interfaces

inline std::ostream& operator<<(std::ostream& os,
                                const core::interfaces::Sensor::GlobalId& id)
{
    return os << id.type() << ": " << id.name();
}
