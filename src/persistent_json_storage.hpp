#pragma once

#include "interfaces/json_storage.hpp"

class PersistentJsonStorage : public interfaces::JsonStorage
{
  public:
    explicit PersistentJsonStorage(const DirectoryPath& directory);

    void store(const FilePath& subPath, const nlohmann::json& data) override;
    bool remove(const FilePath& subPath) override;
    bool isExist(const FilePath& path) const override;
    std::optional<nlohmann::json> load(const FilePath& subPath) const override;
    std::vector<FilePath> list() const override;

  private:
    DirectoryPath directory;

    static std::filesystem::path join(const std::filesystem::path&,
                                      const std::filesystem::path&);
    static void limitPermissions(const std::filesystem::path& path);
};
