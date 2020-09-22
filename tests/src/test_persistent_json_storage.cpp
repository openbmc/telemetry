#include "persistent_json_storage.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace core;
using namespace testing;

class ReportPersistentStorage : public Test
{
  public:
    using FileResource = interfaces::JsonStorage::FileResource;
    using DirectoryResource = interfaces::JsonStorage::DirectoryResource;

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

    const FileResource fileName = FileResource("report/1/file.txt");

    static const DirectoryResource directory;
    PersistentJsonStorage sut{directory};
};

const interfaces::JsonStorage::DirectoryResource
    ReportPersistentStorage::directory =
        interfaces::JsonStorage::DirectoryResource(std::tmpnam(nullptr));

TEST_F(ReportPersistentStorage, storesJsonData)
{
    nlohmann::json data = nlohmann::json::object();
    data["name"] = "kevin";
    data["lastname"] = "mc calister";

    sut.store(fileName, data);

    ASSERT_THAT(sut.load(fileName), Eq(data));
}

TEST_F(ReportPersistentStorage, emptyListWhenNoReportsCreated)
{
    EXPECT_THAT(sut.list(DirectoryResource("report")), SizeIs(0u));
}

TEST_F(ReportPersistentStorage, listSavedReports)
{
    using namespace std::string_literals;

    sut.store(FileResource("report/domain-1/name-1/conf-1.json"),
              nlohmann::json("data-1a"));
    sut.store(FileResource("report/domain-1/name-2/conf-1.json"),
              nlohmann::json("data-2a"));
    sut.store(FileResource("report/domain-1/name-2/conf-2.json"),
              nlohmann::json("data-2b"));
    sut.store(FileResource("report/domain-2/name-1/conf-1.json"),
              nlohmann::json("data-3a"));

    EXPECT_THAT(sut.list(DirectoryResource("report")),
                UnorderedElementsAre(FileResource("report/domain-1/name-1"),
                                     FileResource("report/domain-1/name-2"),
                                     FileResource("report/domain-2/name-1")));
}

TEST_F(ReportPersistentStorage, listSavedReportsWithoutRemovedOnes)
{
    using namespace std::string_literals;

    sut.store(FileResource("report/domain-1/name-1/conf-1.json"),
              nlohmann::json("data-1a"));
    sut.store(FileResource("report/domain-1/name-2/conf-1.json"),
              nlohmann::json("data-2a"));
    sut.store(FileResource("report/domain-1/name-2/conf-2.json"),
              nlohmann::json("data-2b"));
    sut.store(FileResource("report/domain-2/name-1/conf-1.json"),
              nlohmann::json("data-3a"));
    sut.remove(FileResource("report/domain-1/name-1/conf-1.json"));
    sut.remove(FileResource("report/domain-1/name-2/conf-2.json"));

    EXPECT_THAT(sut.list(DirectoryResource("report")),
                UnorderedElementsAre(FileResource("report/domain-1/name-2"),
                                     FileResource("report/domain-2/name-1")));
}

TEST_F(ReportPersistentStorage, removesStoredJson)
{
    nlohmann::json data = nlohmann::json::object();
    data["name"] = "kevin";
    data["lastname"] = "mc calister";

    sut.store(fileName, data);

    ASSERT_THAT(sut.remove(fileName), Eq(true));
    ASSERT_THAT(sut.load(fileName), Eq(std::nullopt));
}

TEST_F(ReportPersistentStorage, returnsFalseWhenDeletingNonExistingFile)
{
    ASSERT_THAT(sut.remove(fileName), Eq(false));
}

TEST_F(ReportPersistentStorage, returnsNulloptWhenFileDoesntExist)
{
    ASSERT_THAT(sut.load(fileName), Eq(std::nullopt));
}
