#include "persistent_json_storage.hpp"

#include "utils/api_error.hpp"
#include "utils/log.hpp"

#include <fstream>

namespace core
{

PersistentJsonStorage::PersistentJsonStorage(
    const DirectoryResource& directory) :
    directory(directory)
{}

void PersistentJsonStorage::store(const FileResource& filePath,
                                  const nlohmann::json& data)
{
    using namespace std::string_literals;

    try
    {
        const auto path = join(directory, filePath);
        std::error_code ec;

        LOG_DEBUG << "Writing file at: " << path;

        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec)
        {
            apiError(std::errc::io_error,
                     "Unable to create directory for file: "s + path.c_str() +
                         ", ec=" + std::to_string(ec.value()) + ": " +
                         ec.message());
        }

        std::ofstream file(path);
        file << data;
        if (!file)
        {
            apiError(std::errc::io_error,
                     "Unable to create file: "s + path.c_str());
        }

        limitPermissions(path.parent_path());
        limitPermissions(path);
    }
    catch (...)
    {
        remove(filePath);
        throw;
    }
}

bool PersistentJsonStorage::remove(const FileResource& filePath)
{
    const auto path = join(directory, filePath);
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
    PersistentJsonStorage::load(const FileResource& filePath) const
{
    const auto path = join(directory, filePath);
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

std::vector<interfaces::JsonStorage::FileResource>
    PersistentJsonStorage::list(const DirectoryResource& subDirectory) const
{
    auto result = std::vector<FileResource>();
    const auto path = join(directory, subDirectory);

    if (!std::filesystem::exists(path))
    {
        return result;
    }

    for (const auto& p : std::filesystem::directory_iterator(path))
    {
        if (std::filesystem::is_directory(p.path()))
        {

            for (auto& item : list(DirectoryResource(p.path())))
            {
                result.emplace_back(std::move(item));
            }
        }
        else
        {
            const auto item = std::filesystem::relative(
                p.path().parent_path(), std::filesystem::path{directory});

            if (std::find(result.begin(), result.end(), item) == result.end())
            {
                result.emplace_back(item);
            }
        }
    }

    return result;
}

std::filesystem::path
    PersistentJsonStorage::join(const std::filesystem::path& left,
                                const std::filesystem::path& right)
{
    return left / right;
}

void PersistentJsonStorage::limitPermissions(const std::filesystem::path& path)
{
    constexpr auto filePerms = std::filesystem::perms::owner_read |
                               std::filesystem::perms::owner_write;
    constexpr auto dirPerms = filePerms | std::filesystem::perms::owner_exec;
    std::filesystem::permissions(
        path, std::filesystem::is_directory(path) ? dirPerms : filePerms,
        std::filesystem::perm_options::replace);
}

} // namespace core
