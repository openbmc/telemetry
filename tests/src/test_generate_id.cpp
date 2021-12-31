#include "utils/generate_id.hpp"

#include <sdbusplus/exception.hpp>

#include <gmock/gmock.h>

using namespace testing;
using namespace std::literals::string_literals;

class ScenarioBase : public Test
{
  public:
    std::string_view defaultName = "defName";
    size_t maxSize = 32;
    std::vector<std::string> conflicts;
};

class ScenarioNameProvided : public ScenarioBase
{
  public:
    auto generateId(std::string_view id, std::string_view name) const
    {
        return utils::generateId(id, name, defaultName, conflicts, maxSize);
    }
};

class TestGenerateIdNameProvided : public ScenarioNameProvided
{};

TEST_F(TestGenerateIdNameProvided, usesIdWhenProvided)
{
    const std::string name = "name";

    EXPECT_THAT(this->generateId("id0", name), Eq(std::pair{"id0"s, name}));
    EXPECT_THAT(this->generateId("/id1", name), Eq(std::pair{"id1"s, name}));
    EXPECT_THAT(this->generateId("prefix/id2", name),
                Eq(std::pair{"prefix/id2"s, name}));
}

TEST_F(TestGenerateIdNameProvided, usedDefaultWhenNothingProvided)
{
    this->defaultName = "def";

    EXPECT_THAT(this->generateId("", ""), Eq(std::pair{"def"s, "def"s}));
    EXPECT_THAT(this->generateId("/", ""), Eq(std::pair{"def"s, "def"s}));
    EXPECT_THAT(this->generateId("abc/", ""),
                Eq(std::pair{"abc/def"s, "def"s}));
}

class ScenarioNameNotProvided : public ScenarioBase
{
  public:
    auto generateId(std::string_view id, std::string_view name) const
    {
        return utils::generateId(id, "", name, conflicts, maxSize);
    }
};

class TestGenerateIdNameNotProvided : public ScenarioNameNotProvided
{};

TEST_F(TestGenerateIdNameNotProvided, usesIdAsNameWhenProvided)
{
    EXPECT_THAT(this->generateId("id0", defaultName),
                Eq(std::pair{"id0"s, "id0"s}));
    EXPECT_THAT(this->generateId("/id1", defaultName),
                Eq(std::pair{"id1"s, "id1"s}));
    EXPECT_THAT(this->generateId("prefix/id2", defaultName),
                Eq(std::pair{"prefix/id2"s, "id2"s}));
}

template <class Scenario>
class TestGenerateId : public Scenario
{};

using TestScenarios =
    ::testing::Types<ScenarioNameProvided, ScenarioNameNotProvided>;
TYPED_TEST_SUITE(TestGenerateId, TestScenarios);

TEST_F(TestGenerateIdNameNotProvided, leadingSlashDoesntCountToLengthLimit)
{
    this->maxSize = 2;

    EXPECT_THAT(this->generateId("12", "default").first, Eq("12"));
    EXPECT_THAT(this->generateId("/23", "default").first, Eq("23"));
    EXPECT_THROW(this->generateId("123", "trigger"),
                 sdbusplus::exception::SdBusError);
}

TYPED_TEST(TestGenerateId, throwsWhenProvidedIdIsTooLong)
{
    this->maxSize = 4;

    EXPECT_THROW(this->generateId("12345", "name"),
                 sdbusplus::exception::SdBusError);
    EXPECT_THROW(this->generateId("12/45", "name"),
                 sdbusplus::exception::SdBusError);
    EXPECT_THROW(this->generateId("123/", "trigger"),
                 sdbusplus::exception::SdBusError);
}

TYPED_TEST(TestGenerateId, throwsWhenThereAreNoFreeIds)
{
    this->maxSize = 1;
    this->conflicts = {"n", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};

    EXPECT_THROW(this->generateId("", "n"), sdbusplus::exception::SdBusError);
}

TYPED_TEST(TestGenerateId, throwsWhenIdContainsMoreThanOneSlash)
{
    EXPECT_THROW(this->generateId("/12/", "name"),
                 sdbusplus::exception::SdBusError);
    EXPECT_THROW(this->generateId("12//", "name"),
                 sdbusplus::exception::SdBusError);
    EXPECT_THROW(this->generateId("12//123", "name"),
                 sdbusplus::exception::SdBusError);
    EXPECT_THROW(this->generateId("12/12/123", "name"),
                 sdbusplus::exception::SdBusError);
}

TYPED_TEST(TestGenerateId, usesNamePartOfNameWhenIdNotProvidedAndNameIsTooLong)
{
    this->maxSize = 4;

    this->conflicts = {};
    EXPECT_THAT(this->generateId("", "trigger"),
                Eq(std::pair{"trig"s, "trigger"s}));
    EXPECT_THAT(this->generateId("a/", "trigger"),
                Eq(std::pair{"a/tr"s, "trigger"s}));

    this->conflicts = {"trig"};
    EXPECT_THAT(this->generateId("", "trigger"),
                Eq(std::pair{"tri0"s, "trigger"s}));

    this->conflicts = {"trig", "tri0"};
    EXPECT_THAT(this->generateId("", "trigger"),
                Eq(std::pair{"tri1"s, "trigger"s}));
}

TYPED_TEST(TestGenerateId, throwsWhenProvidedIdIsTaken)
{
    this->conflicts = {"id", "prefix/id"};

    EXPECT_THROW(this->generateId("id", "name"),
                 sdbusplus::exception::SdBusError);
    EXPECT_THROW(this->generateId("/id", "name"),
                 sdbusplus::exception::SdBusError);
    EXPECT_THROW(this->generateId("prefix/id", "name"),
                 sdbusplus::exception::SdBusError);
}

TYPED_TEST(TestGenerateId, usesNameWhenIdNotProvided)
{
    EXPECT_THAT(this->generateId("", "name"), Eq(std::pair{"name"s, "name"s}));
    EXPECT_THAT(this->generateId("/", "name"), Eq(std::pair{"name"s, "name"s}));
    EXPECT_THAT(this->generateId("abc/", "name"),
                Eq(std::pair{"abc/name"s, "name"s}));
    EXPECT_THAT(this->generateId("123/", "name"),
                Eq(std::pair{"123/name"s, "name"s}));
}

TYPED_TEST(TestGenerateId, usesNameWithoutInvalidCharactersWhenIdNotProvided)
{
    EXPECT_THAT(this->generateId("", "n#a$/m@e"),
                Eq(std::pair{"name"s, "n#a$/m@e"s}));
    EXPECT_THAT(this->generateId("", "n!^aŹ/me"),
                Eq(std::pair{"name"s, "n!^aŹ/me"s}));
    EXPECT_THAT(this->generateId("p/", "n!^aŹ/m*(e"),
                Eq(std::pair{"p/name"s, "n!^aŹ/m*(e"s}));
}
