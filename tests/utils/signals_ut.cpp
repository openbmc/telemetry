#include "signals.hpp"

#include <gmock/gmock.h>

namespace utils::signals
{

using namespace testing;

class SignalsTest : public Test
{
  public:
    auto makeCallback(uint32_t value)
    {
        return [this, value](uint32_t param) { counter += value + param; };
    }

    Signal<void(uint32_t)> sut;
    uint32_t counter = 0u;
};

TEST_F(SignalsTest, callsAllRegisteredCallbacks)
{
    auto connection1 = sut.connect(makeCallback(10u));
    auto connection2 = sut.connect(makeCallback(0u));
    auto connection3 = sut.connect(makeCallback(100u));

    sut(1u);

    ASSERT_THAT(counter, Eq(113u));
}

TEST_F(SignalsTest, doesntCallbackWithNoConnection)
{
    sut.connect(makeCallback(10u));
    auto connection2 = sut.connect(makeCallback(0u));
    auto connection3 = sut.connect(makeCallback(100u));

    sut(1u);

    ASSERT_THAT(counter, Eq(102u));
}

TEST_F(SignalsTest, signalIsDestroyedBeforeConnectionGoesOutOfScope)
{
    Connection connection1, connection2;

    {
        Signal<void(uint32_t)> signal;
        connection1 = signal.connect(makeCallback(10u));
        connection2 = signal.connect(makeCallback(50u));

        signal(1u);
    }

    ASSERT_THAT(counter, Eq(62u));
}

TEST_F(SignalsTest, emittingSignalTwiceCallsMethodsTwice)
{
    auto connection1 = sut.connect(makeCallback(10u));
    auto connection2 = sut.connect(makeCallback(0u));
    auto connection3 = sut.connect(makeCallback(100u));

    sut(1u);
    sut(1u);

    ASSERT_THAT(counter, Eq(226u));
}

} // namespace utils::signals
