#include "persistent_json_storage.hpp"
#include "printers.hpp"

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
        interfaces::JsonStorage::DirectoryPath(std::tmpnam(nullptr));

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
