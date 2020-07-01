#pragma once

#include <string>
#include <vector>

namespace core
{

enum class Persistency
{
    none,
    configurationOnly,
    configurationAndData
};

const std::vector<std::pair<Persistency, std::string>> persistencyConvertData =
    {{Persistency::none, "None"},
     {Persistency::configurationOnly, "ConfigurationOnly"},
     {Persistency::configurationAndData, "ConfigurationAndData"}};

constexpr Persistency defaultPersistency = Persistency::configurationOnly;

} // namespace core
