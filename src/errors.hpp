#pragma once

#include <sdbusplus/exception.hpp>

#include <string>
#include <string_view>

namespace errors
{

class InvalidArgument final : public sdbusplus::exception::exception
{
  public:
    explicit InvalidArgument(std::string_view propertyName);
    InvalidArgument(std::string_view propertyName, std::string_view info);

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
    int get_errno() const noexcept override;

    std::string propertyName;

  private:
    std::string errWhatDetailed;
};

[[noreturn]] void throwInvalidArgument(std::string_view name,
                                       std::string_view info = "");

[[noreturn]] void throwNotAllowed(const std::string& reason);

[[noreturn]] void throwTooManyResources();

} // namespace errors
