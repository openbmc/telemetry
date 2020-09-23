#pragma once

#include "interfaces/json_storage.hpp"

class PersistentJsonStorage : public interfaces::JsonStorage
{
  public:
    explicit PersistentJsonStorage(const DirectoryResource& directory);

    void store(const FileResource& subPath,
               const nlohmann::json& data) override;
    bool remove(const FileResource& subPath) override;
    std::optional<nlohmann::json>
        load(const FileResource& subPath) const override;
    std::vector<FileResource>
        list(const DirectoryResource& subDirectory) const override;

  private:
    DirectoryResource directory;

    static std::filesystem::path join(const std::filesystem::path&,
                                      const std::filesystem::path&);
    static void limitPermissions(const std::filesystem::path& path);
};
