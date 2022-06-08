#include "dbus_environment.hpp"
#include "helpers.hpp"

#include <gmock/gmock.h>

int main(int argc, char** argv)
{
    auto env = new DbusEnvironment;

    testing::InitGoogleTest(&argc, argv);
    testing::AddGlobalTestEnvironment(env);
    auto ret = RUN_ALL_TESTS();

    env->teardown();

    return ret;
}
