#include "errors.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <system_error>

namespace errors
{

using namespace std::literals::string_literals;
using phosphor::logging::elog;
using sdbusplus::error::xyz::openbmc_project::common::NotAllowed;
using sdbusplus::error::xyz::openbmc_project::common::TooManyResources;

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
    return "xyz.openbmc_project.Common.Error.InvalidArgument";
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
    return static_cast<int>(std::errc::io_error);
}

[[noreturn]] void throwInvalidArgument(std::string_view name,
                                       std::string_view info)
{
    if (!info.empty())
    {
        throw InvalidArgument(name, info);
    }

    throw InvalidArgument(name);
}

[[noreturn]] void throwNotAllowed(const std::string& reason)
{
    using Reason =
        phosphor::logging::xyz::openbmc_project::common::NotAllowed::REASON;
    elog<NotAllowed>(Reason(reason.c_str()));
}

[[noreturn]] void throwTooManyResources()
{
    elog<TooManyResources>();
}

} // namespace errors
