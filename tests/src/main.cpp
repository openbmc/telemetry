#include "dbus_environment.hpp"

#include <gmock/gmock.h>

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    testing::AddGlobalTestEnvironment(new DbusEnvironment);
    return RUN_ALL_TESTS();
}
