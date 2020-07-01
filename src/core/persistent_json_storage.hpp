#pragma once

#include "core/interfaces/json_storage.hpp"

#pragma once

namespace core
{

class PersistentStorage : public interfaces::JsonStorage
{
  public:
    explicit PersistentStorage(const std::filesystem::path& directory);

    void store(const std::filesystem::path& subPath,
               const nlohmann::json& data) override;
    bool remove(const std::filesystem::path& subPath) override;
    std::optional<nlohmann::json>
        load(const std::filesystem::path& subPath) const override;
    std::vector<std::filesystem::path>
        list(const std::filesystem::path& subDirectory) const override;

  private:
    std::string directory_;

    static void limitPermissions(const std::filesystem::path& path)
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
