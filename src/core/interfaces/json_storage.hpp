#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <optional>
#include <string>

namespace core::interfaces
{

class JsonStorage
{
  public:
    using Resource = std::filesystem::path;

    virtual ~JsonStorage() = default;

    virtual void store(const Resource& subPath, const nlohmann::json& data) = 0;
    virtual bool remove(const Resource& subPath) = 0;
    virtual std::optional<nlohmann::json>
        load(const Resource& subPath) const = 0;
    virtual std::vector<std::filesystem::path>
        list(const Resource& subDirectory) const = 0;
};

} // namespace core::interfaces
