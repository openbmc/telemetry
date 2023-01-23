#include "helpers.hpp"
#include "persistent_json_storage.hpp"

#include <fstream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace testing;

class TestPersistentJsonStorage : public Test
{
  public:
    using FilePath = interfaces::JsonStorage::FilePath;
    using DirectoryPath = interfaces::JsonStorage::DirectoryPath;

    static void SetUpTestSuite()
    {
        ASSERT_FALSE(std::filesystem::exists(directory));
    }

    void TearDown() override
    {
        if (std::filesystem::exists(directory))
        {
            std::filesystem::remove_all(directory);
        }
    }

    const FilePath fileName = FilePath("report/1/file.txt");

    static const DirectoryPath directory;
    PersistentJsonStorage sut{directory};
};

const interfaces::JsonStorage::DirectoryPath
    TestPersistentJsonStorage::directory =
        interfaces::JsonStorage::DirectoryPath(
            std::filesystem::temp_directory_path() / "telemetry-tests");

TEST_F(TestPersistentJsonStorage, storesJsonData)
{
    nlohmann::json data = nlohmann::json::object();
    data["name"] = "kevin";
    data["lastname"] = "mc calister";

    sut.store(fileName, data);

    ASSERT_THAT(sut.load(fileName), Eq(data));
}

TEST_F(TestPersistentJsonStorage, emptyListWhenNoReportsCreated)
{
    EXPECT_THAT(sut.list(), SizeIs(0u));
}

TEST_F(TestPersistentJsonStorage, listSavedReports)
{
    sut.store(FilePath("report/domain-1/name-1/conf-1.json"),
              nlohmann::json("data-1a"));
    sut.store(FilePath("report/domain-1/name-2/conf-1.json"),
              nlohmann::json("data-2a"));
    sut.store(FilePath("report/domain-1/name-2/conf-2.json"),
              nlohmann::json("data-2b"));
    sut.store(FilePath("report/domain-2/name-1/conf-1.json"),
              nlohmann::json("data-3a"));

    EXPECT_THAT(
        sut.list(),
        UnorderedElementsAre(FilePath("report/domain-1/name-1/conf-1.json"),
                             FilePath("report/domain-1/name-2/conf-1.json"),
                             FilePath("report/domain-1/name-2/conf-2.json"),
                             FilePath("report/domain-2/name-1/conf-1.json")));
}

TEST_F(TestPersistentJsonStorage, listSavedReportsWithoutRemovedOnes)
{
    sut.store(FilePath("report/domain-1/name-1/conf-1.json"),
              nlohmann::json("data-1a"));
    sut.store(FilePath("report/domain-1/name-2/conf-1.json"),
              nlohmann::json("data-2a"));
    sut.store(FilePath("report/domain-1/name-2/conf-2.json"),
              nlohmann::json("data-2b"));
    sut.store(FilePath("report/domain-2/name-1/conf-1.json"),
              nlohmann::json("data-3a"));
    sut.remove(FilePath("report/domain-1/name-1/conf-1.json"));
    sut.remove(FilePath("report/domain-1/name-2/conf-2.json"));

    EXPECT_THAT(
        sut.list(),
        UnorderedElementsAre(FilePath("report/domain-1/name-2/conf-1.json"),
                             FilePath("report/domain-2/name-1/conf-1.json")));
}

TEST_F(TestPersistentJsonStorage, removesStoredJson)
{
    nlohmann::json data = nlohmann::json::object();
    data["name"] = "kevin";
    data["lastname"] = "mc calister";

    sut.store(fileName, data);

    ASSERT_THAT(sut.remove(fileName), Eq(true));
    ASSERT_THAT(sut.load(fileName), Eq(std::nullopt));
}

TEST_F(TestPersistentJsonStorage, returnsFalseWhenDeletingNonExistingFile)
{
    ASSERT_THAT(sut.remove(fileName), Eq(false));
}

TEST_F(TestPersistentJsonStorage, returnsNulloptWhenFileDoesntExist)
{
    ASSERT_THAT(sut.load(fileName), Eq(std::nullopt));
}

struct TestFileSymlink
{
    static interfaces::JsonStorage::FilePath
        setupSymlinks(const std::filesystem::path& originalFile,
                      const interfaces::JsonStorage::DirectoryPath& directory)
    {
        auto linkPath = std::filesystem::path(directory) /
                        "report/symlink.json";
        std::filesystem::create_directories(std::filesystem::path(directory) /
                                            "report");
        std::filesystem::create_symlink(originalFile, linkPath);
        return interfaces::JsonStorage::FilePath(linkPath);
    }
};

struct TestDirectorySymlink
{
    static interfaces::JsonStorage::FilePath
        setupSymlinks(const std::filesystem::path& originalFile,
                      const interfaces::JsonStorage::DirectoryPath& directory)
    {
        auto linkPath = std::filesystem::path(directory) / "reportLink";
        std::filesystem::create_directories(std::filesystem::path(directory) /
                                            "report");
        std::filesystem::create_directory_symlink(originalFile.parent_path(),
                                                  linkPath);
        return interfaces::JsonStorage::FilePath(linkPath /
                                                 originalFile.filename());
    }
};

template <typename T>
class TestPersistentJsonStorageWithSymlink : public TestPersistentJsonStorage
{
  public:
    TestPersistentJsonStorageWithSymlink()
    {
        std::ofstream file(dummyReportPath);
        file << "{}";
        file.close();

        linkPath = T::setupSymlinks(dummyReportPath, directory);
    }

    static void SetUpTestSuite()
    {
        TestPersistentJsonStorage::SetUpTestSuite();
        ASSERT_FALSE(std::filesystem::exists(dummyReportPath));
    }

    void TearDown() override
    {
        TestPersistentJsonStorage::TearDown();
        if (std::filesystem::exists(dummyReportPath))
        {
            std::filesystem::remove(dummyReportPath);
        }
    }

    static const std::filesystem::path dummyReportPath;
    interfaces::JsonStorage::FilePath linkPath;
};

template <typename T>
const std::filesystem::path
    TestPersistentJsonStorageWithSymlink<T>::dummyReportPath =
        std::filesystem::temp_directory_path() / "report.json";

using SymlinkTypes = Types<TestFileSymlink, TestDirectorySymlink>;
TYPED_TEST_SUITE(TestPersistentJsonStorageWithSymlink, SymlinkTypes);

TYPED_TEST(TestPersistentJsonStorageWithSymlink, symlinksAreNotListed)
{
    ASSERT_THAT(TestPersistentJsonStorage::sut.list(), UnorderedElementsAre());
}

TYPED_TEST(TestPersistentJsonStorageWithSymlink, throwsWhenStoreTargetIsSymlink)
{
    ASSERT_THROW(TestPersistentJsonStorage::sut.store(TestFixture::linkPath,
                                                      nlohmann::json("data")),
                 std::runtime_error);

    ASSERT_THAT(TestPersistentJsonStorage::sut.list(), UnorderedElementsAre());
}

TYPED_TEST(TestPersistentJsonStorageWithSymlink,
           returnsNulloptWhenFileIsSymlink)
{
    ASSERT_THAT(TestPersistentJsonStorage::sut.load(TestFixture::linkPath),
                Eq(std::nullopt));
}

TYPED_TEST(TestPersistentJsonStorageWithSymlink,
           returnsFalseWhenTryingToDeleteSymlink)
{
    EXPECT_THAT(TestPersistentJsonStorage::sut.remove(TestFixture::linkPath),
                Eq(false));
    EXPECT_TRUE(std::filesystem::exists(TestFixture::linkPath));
    EXPECT_TRUE(std::filesystem::exists(TestFixture::dummyReportPath));
}
