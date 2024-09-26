#include "helpers.hpp"
#include "utils/dbus_path_utils.hpp"
#include "utils/make_id_name.hpp"
#include "utils/string_utils.hpp"

#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <gmock/gmock.h>

using namespace testing;
using namespace std::literals::string_literals;
using namespace utils::string_utils;

class ScenarioBase : public Test
{
  public:
    std::string_view defaultName = "defName";
    std::vector<std::string> conflicts;
};

class ScenarioNameProvided : public ScenarioBase
{
  public:
    auto makeIdName(std::string_view id, std::string_view name) const
    {
        return utils::makeIdName(id, name, defaultName, conflicts);
    }
};

TEST_F(ScenarioNameProvided, throwsWhenProvidedNameIsTooLong)
{
    EXPECT_THROW(this->makeIdName("", getTooLongName()),
                 errors::InvalidArgument);
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

TEST_F(TestMakeIdNameNameProvided, usedDefaultWhenNameContainsNoIdChars)
{
    this->defaultName = "def";
    const std::string name = " !";

    EXPECT_THAT(this->makeIdName("", name), Eq(std::pair{"def"s, name}));
    EXPECT_THAT(this->makeIdName("prefix/", name),
                Eq(std::pair{"prefix/def"s, name}));
}

class ScenarioNameNotProvided : public ScenarioBase
{
  public:
    auto makeIdName(std::string_view id, std::string_view name) const
    {
        return utils::makeIdName(id, "", name, conflicts);
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
                 errors::InvalidArgument);
    EXPECT_THROW(this->makeIdName("Id/Id%@%!@%!%()%fooo", "name"),
                 errors::InvalidArgument);
    EXPECT_THROW(this->makeIdName("/123", "trigger"), errors::InvalidArgument);
    EXPECT_THROW(this->makeIdName("/123/", "trigger"), errors::InvalidArgument);
}

TYPED_TEST(TestMakeIdName, throwsWhenProvidedIdContainsTooLongSegment)
{
    std::string longPrefix = getTooLongPrefix();
    std::string longSuffix = getTooLongId();
    EXPECT_THROW(this->makeIdName(longPrefix + "/", "name"),
                 errors::InvalidArgument);
    EXPECT_THROW(this->makeIdName(longPrefix + "/Id", "name"),
                 errors::InvalidArgument);
    EXPECT_THROW(this->makeIdName(longPrefix + "/" + longSuffix, "name"),
                 errors::InvalidArgument);
    EXPECT_THROW(this->makeIdName("Prefix/" + longSuffix, "name"),
                 errors::InvalidArgument);
    EXPECT_THROW(this->makeIdName(longSuffix, "name"), errors::InvalidArgument);
}

TYPED_TEST(TestMakeIdName, throwsWhenProvidedIdOrPrefixTooLong)
{
    EXPECT_THROW(this->makeIdName(getTooLongId(), "name"),
                 errors::InvalidArgument);
    EXPECT_THROW(this->makeIdName(getTooLongPrefix() + "/Id", "name"),
                 errors::InvalidArgument);
    EXPECT_THROW(this->makeIdName("Prefix/" + getTooLongId(), "trigger"),
                 errors::InvalidArgument);
}

TYPED_TEST(TestMakeIdName, throwsWhenIdContainsMoreThanOneSlash)
{
    EXPECT_THROW(this->makeIdName("/12/", "name"), errors::InvalidArgument);
    EXPECT_THROW(this->makeIdName("12//", "name"), errors::InvalidArgument);
    EXPECT_THROW(this->makeIdName("12//123", "name"), errors::InvalidArgument);
    EXPECT_THROW(this->makeIdName("12/12/123", "name"),
                 errors::InvalidArgument);
}

TYPED_TEST(TestMakeIdName, usesNameWhenThereAreConflicts)
{
    this->conflicts = {"trigger"};
    EXPECT_THAT(this->makeIdName("", "trigger"),
                Eq(std::pair{"trigger0"s, "trigger"s}));

    this->conflicts = {"trigger", "trigger0"};
    EXPECT_THAT(this->makeIdName("", "trigger"),
                Eq(std::pair{"trigger1"s, "trigger"s}));

    this->conflicts = {getMaxId()};
    std::string expectedId = getMaxId();
    expectedId[expectedId.length() - 1] = '0';
    EXPECT_THAT(this->makeIdName("", getMaxId()),
                Eq(std::pair{expectedId, getMaxId()}));
}

TYPED_TEST(TestMakeIdName, throwsWhenProvidedIdIsTaken)
{
    using sdbusplus::error::xyz::openbmc_project::common::NotAllowed;
    this->conflicts = {"id", "prefix/id"};

    EXPECT_THROW(this->makeIdName("id", "name"), NotAllowed);
    EXPECT_THROW(this->makeIdName("prefix/id", "name"), NotAllowed);
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
