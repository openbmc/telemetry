#include "persistent_storage.hpp"

#include "log.hpp"
#include "utils.hpp"

#include <fstream>

namespace core
{

PersistentStorage::PersistentStorage(const std::filesystem::path& directory) :
    directory_(directory)
{}

void PersistentStorage::store(const std::filesystem::path& subPath,
                              const nlohmann::json& data)
{
    try
    {
        const auto path = directory_ / subPath;
        std::error_code ec;

        LOG_DEBUG << "Writing file at: " << path;

        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec)
        {
            utils::throwError(
                std::errc::io_error,
                std::string("Unable to create directory for file: ") +
                    path.c_str() + ", ec=" + std::to_string(ec.value()) + ": " +
                    ec.message());
        }

        std::ofstream file(path);
        file << data;
        if (!file)
        {
            utils::throwError(std::errc::io_error,
                              std::string("Unable to create file: ") +
                                  path.c_str());
        }

        limitPermissions(path.parent_path());
        limitPermissions(path);
    }
    catch (...)
    {
        remove(subPath);
        throw;
    }
}

bool PersistentStorage::remove(const std::filesystem::path& subPath)
{
    const auto path = directory_ / subPath;
    std::error_code ec;

    auto removed = std::filesystem::remove(path, ec);
    if (!removed)
    {
        LOG_ERROR << "Unable to remove file: " << path << ", ec=" << ec.value()
                  << ": " << ec.message();
        return false;
    }

    /* removes directory only if it is empty */
    std::filesystem::remove(path.parent_path(), ec);

    return true;
}

std::optional<nlohmann::json>
    PersistentStorage::load(const std::filesystem::path& subPath) const
{
    const auto path = directory_ / subPath;
    if (!std::filesystem::exists(path))
    {
        return std::nullopt;
    }

    nlohmann::json result;

    try
    {
        std::ifstream file(path);
        file >> result;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << e.what();
        return std::nullopt;
    }

    return result;
}

std::vector<std::filesystem::path>
    PersistentStorage::list(const std::filesystem::path& subDirectory) const
{
    auto result = std::vector<std::filesystem::path>();

    if (!std::filesystem::exists(directory_ / subDirectory))
    {
        return result;
    }

    for (const auto& p :
         std::filesystem::directory_iterator(directory_ / subDirectory))
    {
        if (std::filesystem::is_directory(p.path()))
        {
            for (auto& item : list(subDirectory / p.path().filename()))
            {
                result.emplace_back(std::move(item));
            }
        }
        else
        {
            const auto item =
                std::filesystem::relative(p.path().parent_path(), directory_);

            if (std::find(std::begin(result), std::end(result), item) ==
                std::end(result))
            {
                result.emplace_back(item);
            }
        }
    }

    return result;
}

} // namespace core
