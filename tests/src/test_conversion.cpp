#include "utils/conversion.hpp"

#include <gmock/gmock.h>

using namespace testing;

class TestConversion : public Test
{
  public:
    enum class Enum
    {
        zero = 0,
        one,
        two
    };

    Enum toEnum(int x)
    {
        return utils::toEnum<Enum, Enum::zero, Enum::two>(x);
    }

    Enum stringToEnum(const std::string& value)
    {
        return utils::stringToEnum(convDataEnum, value);
    }

    std::string enumToString(Enum value)
    {
        return std::string(utils::enumToString(convDataEnum, value));
    }

    static constexpr std::array<std::pair<std::string_view, Enum>, 3>
        convDataEnum = {
            {std::make_pair<std::string_view, Enum>("zero", Enum::zero),
             std::make_pair<std::string_view, Enum>("one", Enum::one),
             std::make_pair<std::string_view, Enum>("two", Enum::two)}};
};

TEST_F(TestConversion, passValueInRangeExpectToGetValidOutput)
{
    EXPECT_EQ(toEnum(0), Enum::zero);
    EXPECT_EQ(toEnum(2), Enum::two);
}

TEST_F(TestConversion, passInvalidValueExpectToThrowOutOfRangeException)
{
    EXPECT_THROW(toEnum(-1), std::out_of_range);
    EXPECT_THROW(toEnum(3), std::out_of_range);
}

TEST_F(TestConversion, convertsToUnderlyingType)
{
    EXPECT_THAT(utils::toUnderlying(Enum::one), Eq(1));
    EXPECT_THAT(utils::toUnderlying(Enum::two), Eq(2));
    EXPECT_THAT(utils::toUnderlying(Enum::zero), Eq(0));
}

TEST_F(TestConversion, convertsEnumToString)
{
    EXPECT_THAT(enumToString(Enum::one), Eq("one"));
    EXPECT_THAT(enumToString(Enum::two), Eq("two"));
    EXPECT_THAT(enumToString(Enum::zero), Eq("zero"));
}

TEST_F(TestConversion, convertsStringToEnum)
{
    EXPECT_THAT(stringToEnum("one"), Eq(Enum::one));
    EXPECT_THAT(stringToEnum("two"), Eq(Enum::two));
    EXPECT_THAT(stringToEnum("zero"), Eq(Enum::zero));
}

TEST_F(TestConversion, enumToStringThrowsWhenUknownEnumPassed)
{
    EXPECT_THROW(enumToString(static_cast<Enum>(77)), std::out_of_range);
}

TEST_F(TestConversion, stringToEnumThrowsWhenUknownStringPassed)
{
    EXPECT_THROW(stringToEnum("four"), std::out_of_range);
}
