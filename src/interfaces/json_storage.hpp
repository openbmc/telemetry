#pragma once

#include <boost/serialization/strong_typedef.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <optional>
#include <string>

namespace interfaces
{

class JsonStorage
{
  public:
    BOOST_STRONG_TYPEDEF(std::filesystem::path, FilePath)
    BOOST_STRONG_TYPEDEF(std::filesystem::path, DirectoryPath)

    virtual ~JsonStorage() = default;

    virtual void store(const FilePath& subPath, const nlohmann::json& data) = 0;
    virtual bool remove(const FilePath& subPath) = 0;
    virtual std::optional<nlohmann::json>
        load(const FilePath& subPath) const = 0;
    virtual std::vector<FilePath>
        list(const DirectoryPath& subDirectory) const = 0;
};

} // namespace interfaces
