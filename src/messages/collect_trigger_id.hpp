#pragma once

#include <string>

namespace messages
{

struct CollectTriggerIdReq
{
    std::string reportId;
};

struct CollectTriggerIdResp
{
    std::string triggerId;
};

} // namespace messages
