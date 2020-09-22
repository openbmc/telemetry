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
    BOOST_STRONG_TYPEDEF(std::filesystem::path, FileResource)
    BOOST_STRONG_TYPEDEF(std::filesystem::path, DirectoryResource)

    virtual ~JsonStorage() = default;

    virtual void store(const FileResource& subPath,
                       const nlohmann::json& data) = 0;
    virtual bool remove(const FileResource& subPath) = 0;
    virtual std::optional<nlohmann::json>
        load(const FileResource& subPath) const = 0;
    virtual std::vector<FileResource>
        list(const DirectoryResource& subDirectory) const = 0;
};

} // namespace interfaces
