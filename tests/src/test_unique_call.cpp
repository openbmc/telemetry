#include "helpers.hpp"
#include "utils/unique_call.hpp"

#include <gmock/gmock.h>

namespace utils
{

using namespace testing;

class TestUniqueCall : public Test
{
  public:
    void uniqueCallIncrementCounter()
    {
        uniqueCall1([this](auto context) { ++counter; });
    }

    void uniqueCallWhileUniqueCallIsActiveIncrementCounter()
    {
        uniqueCall2([this](auto context) {
            ++counter;
            uniqueCallWhileUniqueCallIsActiveIncrementCounter();
        });
    }

    UniqueCall uniqueCall1;
    UniqueCall uniqueCall2;
    uint32_t counter = 0u;
};

TEST_F(TestUniqueCall, shouldExecuteNormally)
{
    for (size_t i = 0; i < 3u; ++i)
    {
        uniqueCallIncrementCounter();
    }

    ASSERT_THAT(counter, Eq(3u));
}

TEST_F(TestUniqueCall, shouldNotExecuteWhenPreviousExecutionIsStillActive)
{
    for (size_t i = 0; i < 3u; ++i)
    {
        uniqueCallWhileUniqueCallIsActiveIncrementCounter();
    }

    ASSERT_THAT(counter, Eq(3u));
}

} // namespace utils
