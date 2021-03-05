#pragma once

#include "interfaces/types.hpp"

#include <ostream>

#include <gmock/gmock.h>

namespace utils
{

inline void PrintTo(const LabeledSensorParameters& o, std::ostream* os)
{
    using testing::PrintToString;

    (*os) << "{ ";
    (*os) << utils::tstring::Service::str() << ": "
          << PrintToString(o.at_label<utils::tstring::Service>()) << ", ";
    (*os) << utils::tstring::Path::str() << ": "
          << PrintToString(o.at_label<utils::tstring::Path>());
    (*os) << " }";
}

} // namespace utils