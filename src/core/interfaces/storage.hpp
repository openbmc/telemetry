#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <optional>
#include <string>

namespace core::interfaces
{

class Storage
{
  public:
    virtual ~Storage() = default;

    virtual void store(const std::filesystem::path& subPath,
                       const nlohmann::json& data) = 0;
    virtual bool remove(const std::filesystem::path& subPath) = 0;
    virtual std::optional<nlohmann::json>
        load(const std::filesystem::path& subPath) const = 0;
    virtual std::vector<std::filesystem::path>
        list(const std::filesystem::path& subDirectory) const = 0;
};

} // namespace core::interfaces
