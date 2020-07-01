#include "core/persistent_storage.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace core;
using namespace testing;

class ReportPersistentStorage : public Test
{
  public:
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

    const std::filesystem::path fileName = "report/1/file.txt";

    static const std::filesystem::path directory;
    PersistentStorage sut{directory};
};

const std::filesystem::path ReportPersistentStorage::directory =
    std::tmpnam(nullptr);

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
    EXPECT_THAT(sut.list("report"), SizeIs(0u));
}

TEST_F(ReportPersistentStorage, listSavedReports)
{
    using namespace std::string_literals;

    sut.store("report/domain-1/name-1/conf-1.json", nlohmann::json("data-1a"));
    sut.store("report/domain-1/name-2/conf-1.json", nlohmann::json("data-2a"));
    sut.store("report/domain-1/name-2/conf-2.json", nlohmann::json("data-2b"));
    sut.store("report/domain-2/name-1/conf-1.json", nlohmann::json("data-3a"));

    EXPECT_THAT(sut.list("report"),
                UnorderedElementsAre("report/domain-1/name-1"s,
                                     "report/domain-1/name-2"s,
                                     "report/domain-2/name-1"s));
}

TEST_F(ReportPersistentStorage, listSavedReportsWithoutRemovedOnes)
{
    using namespace std::string_literals;

    sut.store("report/domain-1/name-1/conf-1.json", nlohmann::json("data-1a"));
    sut.store("report/domain-1/name-2/conf-1.json", nlohmann::json("data-2a"));
    sut.store("report/domain-1/name-2/conf-2.json", nlohmann::json("data-2b"));
    sut.store("report/domain-2/name-1/conf-1.json", nlohmann::json("data-3a"));
    sut.remove("report/domain-1/name-1/conf-1.json");
    sut.remove("report/domain-1/name-2/conf-2.json");

    EXPECT_THAT(sut.list("report"),
                UnorderedElementsAre("report/domain-1/name-2"s,
                                     "report/domain-2/name-1"s));
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
