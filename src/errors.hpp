#pragma once

#include <sdbusplus/exception.hpp>

#include <string>
#include <string_view>

namespace errors
{

class InvalidArgument final : public sdbusplus::exception::internal_exception
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

} // namespace errors
