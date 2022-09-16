#include "errors.hpp"

#include <system_error>

namespace errors
{

using namespace std::literals::string_literals;

InvalidArgument::InvalidArgument(std::string_view propertyNameArg) :
    propertyName(propertyNameArg),
    errWhatDetailed("Invalid argument was given for property: "s +
                    description())
{}

InvalidArgument::InvalidArgument(std::string_view propertyNameArg,
                                 std::string_view info) :
    propertyName(propertyNameArg),
    errWhatDetailed(
        ("Invalid argument was given for property: "s + description() + ". "s)
            .append(info))
{}

const char* InvalidArgument::name() const noexcept
{
    return "org.freedesktop.DBus.Error.InvalidArgs";
}

const char* InvalidArgument::description() const noexcept
{
    return propertyName.c_str();
}

const char* InvalidArgument::what() const noexcept
{
    return errWhatDetailed.c_str();
}

int InvalidArgument::get_errno() const noexcept
{
    return static_cast<int>(std::errc::invalid_argument);
}

} // namespace errors
