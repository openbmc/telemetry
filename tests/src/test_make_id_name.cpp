#include "helpers.hpp"
#include "utils/dbus_path_utils.hpp"
#include "utils/make_id_name.hpp"

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
    auto makeIdName(std::string_view id, std::string_view name) const
    {
        return utils::makeIdName(id, name, defaultName, conflicts, maxSize);
    }
};

TEST_F(ScenarioNameProvided, throwsWhenProvidedNameIsTooLong)
{
    std::string longName(utils::constants::maxIdNameLength + 1, 'E');
    EXPECT_THROW(this->makeIdName("", longName),
                 sdbusplus::exception::SdBusError);
}

class TestMakeIdNameNameProvided : public ScenarioNameProvided
{};

TEST_F(TestMakeIdNameNameProvided, usesIdWhenProvided)
{
    const std::string name = "name";

    EXPECT_THAT(this->makeIdName("id0", name), Eq(std::pair{"id0"s, name}));
    EXPECT_THAT(this->makeIdName("prefix/id2", name),
                Eq(std::pair{"prefix/id2"s, name}));
}

TEST_F(TestMakeIdNameNameProvided, usedDefaultWhenNothingProvided)
{
    this->defaultName = "def";

    EXPECT_THAT(this->makeIdName("", ""), Eq(std::pair{"def"s, "def"s}));
    EXPECT_THAT(this->makeIdName("abc/", ""),
                Eq(std::pair{"abc/def"s, "def"s}));
}

class ScenarioNameNotProvided : public ScenarioBase
{
  public:
    auto makeIdName(std::string_view id, std::string_view name) const
    {
        return utils::makeIdName(id, "", name, conflicts, maxSize);
    }
};

class TestMakeIdNameNameNotProvided : public ScenarioNameNotProvided
{};

TEST_F(TestMakeIdNameNameNotProvided, usesIdAsNameWhenProvided)
{
    EXPECT_THAT(this->makeIdName("id0", defaultName),
                Eq(std::pair{"id0"s, "id0"s}));
    EXPECT_THAT(this->makeIdName("prefix/id2", defaultName),
                Eq(std::pair{"prefix/id2"s, "id2"s}));
}

template <class Scenario>
class TestMakeIdName : public Scenario
{};

using TestScenarios =
    ::testing::Types<ScenarioNameProvided, ScenarioNameNotProvided>;
TYPED_TEST_SUITE(TestMakeIdName, TestScenarios);

TYPED_TEST(TestMakeIdName, throwsWhenProvidedIdContainsIncorrectCharacters)
{
    EXPECT_THROW(this->makeIdName("Id%@%!@%!%()%fooo/Id", "name"),
                 sdbusplus::exception::SdBusError);
    EXPECT_THROW(this->makeIdName("Id/Id%@%!@%!%()%fooo", "name"),
                 sdbusplus::exception::SdBusError);
    EXPECT_THROW(this->makeIdName("/123", "trigger"),
                 sdbusplus::exception::SdBusError);
    EXPECT_THROW(this->makeIdName("/123/", "trigger"),
                 sdbusplus::exception::SdBusError);
}

TYPED_TEST(TestMakeIdName, throwsWhenProvidedIdContainsTooLongSegment)
{
    std::string longPrefix(utils::constants::maxPrefixLength + 1, 'E');
    std::string longSuffix(utils::constants::maxIdNameLength + 1, 'E');
    EXPECT_THROW(this->makeIdName(longPrefix + "/", "name"),
                 sdbusplus::exception::SdBusError);
    EXPECT_THROW(this->makeIdName(longPrefix + "/Id", "name"),
                 sdbusplus::exception::SdBusError);
    EXPECT_THROW(this->makeIdName(longPrefix + "/" + longSuffix, "name"),
                 sdbusplus::exception::SdBusError);
    EXPECT_THROW(this->makeIdName("Prefix/" + longSuffix, "name"),
                 sdbusplus::exception::SdBusError);
    EXPECT_THROW(this->makeIdName(longSuffix, "name"),
                 sdbusplus::exception::SdBusError);
}

TYPED_TEST(TestMakeIdName, throwsWhenProvidedIdIsTooLong)
{
    this->maxSize = 4;

    EXPECT_THROW(this->makeIdName("12345", "name"),
                 sdbusplus::exception::SdBusError);
    EXPECT_THROW(this->makeIdName("12/45", "name"),
                 sdbusplus::exception::SdBusError);
    EXPECT_THROW(this->makeIdName("123/", "trigger"),
                 sdbusplus::exception::SdBusError);
}

TYPED_TEST(TestMakeIdName, throwsWhenThereAreNoFreeIds)
{
    this->maxSize = 1;
    this->conflicts = {"n", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};

    EXPECT_THROW(this->makeIdName("", "n"), sdbusplus::exception::SdBusError);
}

TYPED_TEST(TestMakeIdName, throwsWhenIdContainsMoreThanOneSlash)
{
    EXPECT_THROW(this->makeIdName("/12/", "name"),
                 sdbusplus::exception::SdBusError);
    EXPECT_THROW(this->makeIdName("12//", "name"),
                 sdbusplus::exception::SdBusError);
    EXPECT_THROW(this->makeIdName("12//123", "name"),
                 sdbusplus::exception::SdBusError);
    EXPECT_THROW(this->makeIdName("12/12/123", "name"),
                 sdbusplus::exception::SdBusError);
}

TYPED_TEST(TestMakeIdName, usesNamePartOfNameWhenIdNotProvidedAndNameIsTooLong)
{
    this->maxSize = 4;

    this->conflicts = {};
    EXPECT_THAT(this->makeIdName("", "trigger"),
                Eq(std::pair{"trig"s, "trigger"s}));
    EXPECT_THAT(this->makeIdName("a/", "trigger"),
                Eq(std::pair{"a/tr"s, "trigger"s}));

    this->conflicts = {"trig"};
    EXPECT_THAT(this->makeIdName("", "trigger"),
                Eq(std::pair{"tri0"s, "trigger"s}));

    this->conflicts = {"trig", "tri0"};
    EXPECT_THAT(this->makeIdName("", "trigger"),
                Eq(std::pair{"tri1"s, "trigger"s}));
}

TYPED_TEST(TestMakeIdName, throwsWhenProvidedIdIsTaken)
{
    this->conflicts = {"id", "prefix/id"};

    EXPECT_THROW(this->makeIdName("id", "name"),
                 sdbusplus::exception::SdBusError);
    EXPECT_THROW(this->makeIdName("prefix/id", "name"),
                 sdbusplus::exception::SdBusError);
}

TYPED_TEST(TestMakeIdName, usesNameWhenIdNotProvided)
{
    EXPECT_THAT(this->makeIdName("", "name"), Eq(std::pair{"name"s, "name"s}));
    EXPECT_THAT(this->makeIdName("abc/", "name"),
                Eq(std::pair{"abc/name"s, "name"s}));
    EXPECT_THAT(this->makeIdName("123/", "name"),
                Eq(std::pair{"123/name"s, "name"s}));
}

TYPED_TEST(TestMakeIdName, usesNameWithoutInvalidCharactersWhenIdNotProvided)
{
    EXPECT_THAT(this->makeIdName("", "n#a$/m@e"),
                Eq(std::pair{"name"s, "n#a$/m@e"s}));
    EXPECT_THAT(this->makeIdName("", "n!^aŹ/me"),
                Eq(std::pair{"name"s, "n!^aŹ/me"s}));
    EXPECT_THAT(this->makeIdName("p/", "n!^aŹ/m*(e"),
                Eq(std::pair{"p/name"s, "n!^aŹ/m*(e"s}));
}
