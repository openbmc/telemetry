#pragma once

#include "error_type.hpp"

#include <string>

struct ErrorMessage
{
    ErrorType error;
    std::string arg0;
};
