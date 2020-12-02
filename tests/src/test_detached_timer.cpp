#include "dbus_environment.hpp"
#include "helpers.hpp"
#include "utils/detached_timer.hpp"

#include <gmock/gmock.h>

namespace utils
{

using namespace testing;
using namespace std::chrono_literals;

class TestDetachedTimer : public Test
{
  public:
    uint32_t value = 0;
};

TEST_F(TestDetachedTimer, executesLambdaAfterTimeout)
{
    auto setPromise = DbusEnvironment::setPromise("timer");

    makeDetachedTimer(DbusEnvironment::getIoc(), 100ms, [this, &setPromise] {
        ++value;
        setPromise();
    });

    auto elapsed = DbusEnvironment::measureTime(
        [] { DbusEnvironment::waitForFuture("timer"); });

    EXPECT_THAT(elapsed, AllOf(Ge(100ms), Lt(200ms)));
    EXPECT_THAT(value, Eq(1u));
}

} // namespace utils
