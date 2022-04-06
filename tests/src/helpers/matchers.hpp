#include <gmock/gmock.h>

using namespace testing;

namespace helpers
{

MATCHER_P(IsValueOfOptionalRefEq, expectedValue, "")
{
    if (arg)
    {
        *result_listener << "where the value is \"" << arg->get() << "\"";
        return arg->get() == expectedValue;
    }
    *result_listener << "where there is no value";
    return false;
}

MATCHER(IsNoValueOfOptionalRef, "")
{
    if (arg)
    {
        *result_listener << "where the value is \"" << arg->get() << "\"";
        return false;
    }
    return true;
}

} // namespace helpers
