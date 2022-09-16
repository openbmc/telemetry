#include "helpers.hpp"
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
        return utils::toEnum<Enum, utils::minEnumValue(convDataEnum),
                             utils::maxEnumValue(convDataEnum)>(x);
    }

    Enum toEnum(const std::string& value)
    {
        return utils::toEnum(convDataEnum, value);
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

namespace utils
{

template <>
struct EnumTraits<TestConversion::Enum>
{
    static constexpr auto propertyName = ConstexprString{"Enum"};
};

} // namespace utils

TEST_F(TestConversion, passValueInRangeExpectToGetValidOutput)
{
    EXPECT_EQ(toEnum(0), Enum::zero);
    EXPECT_EQ(toEnum(2), Enum::two);
}

TEST_F(TestConversion, passInvalidValueExpectToThrowException)
{
    EXPECT_THROW(toEnum(-1), errors::InvalidArgument);
    EXPECT_THROW(toEnum(3), errors::InvalidArgument);
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
    EXPECT_THAT(toEnum("one"), Eq(Enum::one));
    EXPECT_THAT(toEnum("two"), Eq(Enum::two));
    EXPECT_THAT(toEnum("zero"), Eq(Enum::zero));
}

TEST_F(TestConversion, enumToStringThrowsWhenUknownEnumPassed)
{
    EXPECT_THROW(enumToString(static_cast<Enum>(77)), errors::InvalidArgument);
}

TEST_F(TestConversion, toEnumThrowsWhenUknownStringPassed)
{
    EXPECT_THROW(toEnum("four"), errors::InvalidArgument);
}
