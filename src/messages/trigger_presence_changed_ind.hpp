#pragma once

#include "messages/presence.hpp"

#include <string>
#include <vector>

namespace messages
{

struct TriggerPresenceChangedInd
{
    Presence presence;
    std::string triggerId;
    std::vector<std::string> reportIds;
};

} // namespace messages
