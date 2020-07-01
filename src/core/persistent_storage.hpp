#pragma once

#include "core/interfaces/storage.hpp"

#pragma once

namespace core
{

class PersistentStorage : public interfaces::Storage
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
};

} // namespace core
