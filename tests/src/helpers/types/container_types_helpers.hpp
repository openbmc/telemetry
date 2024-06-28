#pragma once

#include <iostream>
#include <vector>

#include <gmock/gmock.h>

template <class Param>
void PrintTo(const std::vector<Param>& vec, std::ostream* os)
{
    *os << "[ ";
    bool first = true;
    for (const auto& item : vec)
    {
        if (!first)
        {
            *os << ", ";
        }
        PrintTo(item, os);
        first = false;
    }
    *os << " ]";
}
