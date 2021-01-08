#include "utils/conversion.hpp"

#include <gmock/gmock.h>

using namespace testing;

class TestEnum : public Test
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
};

TEST_F(TestEnum, passValueInRangeExpectToGetValidOutput)
{
    EXPECT_EQ(toEnum(0), Enum::zero);
    EXPECT_EQ(toEnum(2), Enum::two);
}

TEST_F(TestEnum, passInvalidValueExpectToThrowOutOfRangeException)
{
    EXPECT_THROW(toEnum(-1), std::out_of_range);
    EXPECT_THROW(toEnum(3), std::out_of_range);
}
