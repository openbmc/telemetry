#pragma once

#include "utils/labeled_tuple.hpp"
#include "utils/tstring.hpp"

#include <tuple>
#include <vector>

using SensorsInfo =
    std::vector<std::pair<sdbusplus::message::object_path, std::string>>;

using LabeledSensorInfo =
    utils::LabeledTuple<std::tuple<std::string, std::string, std::string>,
                        utils::tstring::Service, utils::tstring::Path,
                        utils::tstring::Metadata>;
