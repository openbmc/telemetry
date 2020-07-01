#pragma once

#include "core/interfaces/json_storage.hpp"

#pragma once

namespace core
{

class PersistentStorage : public interfaces::JsonStorage
{
  public:
    explicit PersistentStorage(const Resource& directory);

    void store(const Resource& subPath, const nlohmann::json& data) override;
    bool remove(const Resource& subPath) override;
    std::optional<nlohmann::json> load(const Resource& subPath) const override;
    std::vector<Resource> list(const Resource& subDirectory) const override;

  private:
    std::string directory_;

    static void limitPermissions(const Resource& path)
    {
        constexpr auto filePerms = std::filesystem::perms::owner_read |
                                   std::filesystem::perms::owner_write;
        constexpr auto dirPerms =
            filePerms | std::filesystem::perms::owner_exec;
        std::filesystem::permissions(
            path, std::filesystem::is_directory(path) ? dirPerms : filePerms,
            std::filesystem::perm_options::replace);
    }
};

} // namespace core
