#pragma once

#include "error_type.hpp"

#include <string>

struct ErrorMessage
{
    ErrorType error;
    std::string arg0;

    ErrorMessage(ErrorType errorIn, std::string_view arg0In) :
        error(errorIn), arg0(arg0In)
    {}
};
