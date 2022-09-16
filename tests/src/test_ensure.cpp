#include "helpers.hpp"
#include "utils/ensure.hpp"

#include <gmock/gmock.h>

using namespace testing;

class TestEnsure : public Test
{
  public:
    utils::Ensure<std::function<void()>> sut;

    size_t executed = 0u;
};

TEST_F(TestEnsure, executesCallbackOnceWhenDestroyed)
{
    sut = [this] { ++executed; };
    sut = nullptr;

    EXPECT_THAT(executed, Eq(1u));
}

TEST_F(TestEnsure, executesCallbackTwiceWhenNewCallbackAssigned)
{
    sut = [this] { ++executed; };
    sut = [this] { executed += 10; };
    sut = nullptr;

    EXPECT_THAT(executed, Eq(11u));
}
