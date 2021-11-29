#include "utils/generate_id.hpp"

#include <gmock/gmock.h>

using namespace testing;

class TestGenerateId : public Test
{
  public:
    std::vector<std::string> conflicts;
};

TEST_F(TestGenerateId, returnPrefixWhenPrefixIsId)
{
    constexpr size_t maxSize = 32;
    EXPECT_THAT(utils::generateId("prefix", "name", conflicts, maxSize),
                Eq("prefix"));
    EXPECT_THAT(utils::generateId("pre", "name123", conflicts, maxSize),
                Eq("pre"));
    EXPECT_THAT(utils::generateId("prefix/abc", "name", conflicts, maxSize),
                Eq("prefix/abc"));
    EXPECT_THAT(utils::generateId("prefix/abc", "name",
                                  {"conflicts", "prefix/abc"}, maxSize),
                Eq("prefix/abc"));
}

TEST_F(TestGenerateId, returnsNameWithPrefixWhenPrefixIsNamesapce)
{
    constexpr size_t maxSize = 32;
    EXPECT_THAT(utils::generateId("prefix/", "name", conflicts, maxSize),
                Eq("prefix/name"));
    EXPECT_THAT(utils::generateId("pre/", "name", conflicts, maxSize),
                Eq("pre/name"));
}

TEST_F(TestGenerateId, returnsOriginalNameWithoutInvalidCharacters)
{
    constexpr size_t maxSize = 32;
    EXPECT_THAT(utils::generateId("", "n#a$m@e", conflicts, maxSize),
                Eq("name"));
    EXPECT_THAT(utils::generateId("/", "n!^aŹme", conflicts, maxSize),
                Eq("/name"));
    EXPECT_THAT(utils::generateId("p/", "n!^aŹm*(e", conflicts, maxSize),
                Eq("p/name"));
}

TEST_F(TestGenerateId, returnsUniqueIdWhenConflictExist)
{
    constexpr size_t maxSize = 32;

    EXPECT_THAT(utils::generateId("p/", "name",
                                  {"conflicts", "p/name", "p/name1"}, maxSize),
                Eq("p/name0"));
    EXPECT_THAT(utils::generateId("p/", "name",
                                  {"conflicts", "p/name", "p/name0", "p/name1"},
                                  maxSize),
                Eq("p/name2"));
}

TEST_F(TestGenerateId, returnsUniqueIdWithingMaxSize)
{
    constexpr size_t maxSize = 4;

    EXPECT_THAT(utils::generateId("", "trigger", {""}, maxSize), Eq("trig"));
    EXPECT_THAT(utils::generateId("", "trigger", {"trig"}, maxSize),
                Eq("tri0"));
}
