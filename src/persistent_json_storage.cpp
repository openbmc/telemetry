#include "persistent_json_storage.hpp"

#include <phosphor-logging/log.hpp>

#include <fstream>
#include <stdexcept>

PersistentJsonStorage::PersistentJsonStorage(const DirectoryPath& directory) :
    directory(directory)
{}

void PersistentJsonStorage::store(const FilePath& filePath,
                                  const nlohmann::json& data)
{
    try
    {
        const auto path = join(directory, filePath);
        std::error_code ec;

        phosphor::logging::log<phosphor::logging::level::DEBUG>(
            "Store to file", phosphor::logging::entry("PATH=%s", path.c_str()));

        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec)
        {
            throw std::runtime_error(
                "Unable to create directory for file: " + path.string() +
                ", ec=" + std::to_string(ec.value()) + ": " + ec.message());
        }

        assertThatPathIsNotSymlink(path);

        std::ofstream file(path);
        file << data;
        if (!file)
        {
            throw std::runtime_error("Unable to create file: " + path.string());
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

bool PersistentJsonStorage::remove(const FilePath& filePath)
{
    const auto path = join(directory, filePath);

    if (std::filesystem::is_symlink(path))
    {
        return false;
    }

    std::error_code ec;

    auto removed = std::filesystem::remove(path, ec);
    if (!removed)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Unable to remove file",
            phosphor::logging::entry("PATH=%s", path.c_str()),
            phosphor::logging::entry("ERROR_CODE=%d", ec.value()));
        return false;
    }

    /* removes directory only if it is empty */
    std::filesystem::remove(path.parent_path(), ec);

    return true;
}

std::optional<nlohmann::json>
    PersistentJsonStorage::load(const FilePath& filePath) const
{
    const auto path = join(directory, filePath);
    if (!std::filesystem::exists(path))
    {
        return std::nullopt;
    }

    nlohmann::json result;

    try
    {
        assertThatPathIsNotSymlink(path);
        std::ifstream file(path);
        file >> result;
    }
    catch (const std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
        return std::nullopt;
    }

    return result;
}

std::vector<interfaces::JsonStorage::FilePath>
    PersistentJsonStorage::list() const
{
    std::vector<interfaces::JsonStorage::FilePath> result;
    if (!std::filesystem::exists(directory))
    {
        return result;
    }

    for (const auto& p :
         std::filesystem::recursive_directory_iterator(directory))
    {
        if (p.is_regular_file())
        {
            auto item = std::filesystem::relative(p.path(), directory);
            result.emplace_back(std::move(item));
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

bool PersistentJsonStorage::exist(const FilePath& subPath) const
{
    return std::filesystem::exists(join(directory, subPath));
}

void PersistentJsonStorage::assertThatPathIsNotSymlink(
    const std::filesystem::path& path)
{
    if (std::filesystem::is_symlink(path))
    {
        throw std::runtime_error("Source/Target file is a symlink!");
    }
}
