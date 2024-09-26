#include "errors.hpp"
#include "helpers.hpp"
#include "utils/dbus_path_utils.hpp"

#include <gmock/gmock.h>

using namespace testing;
using sdbusplus::message::object_path;
using utils::pathAppend;

class TestPathAppend :
    public Test,
    public WithParamInterface<std::tuple<object_path, std::string, object_path>>
{};

INSTANTIATE_TEST_SUITE_P(
    _, TestPathAppend,
    Values(std::make_tuple(object_path("/Base/Path"), "one",
                           object_path("/Base/Path/one")),
           std::make_tuple(object_path("/Base/Path"), "one/two",
                           object_path("/Base/Path/one/two")),
           std::make_tuple(object_path("/Base/Path"), "one/two/foobar",
                           object_path("/Base/Path/one/two/foobar")),
           std::make_tuple(object_path("/Base/Path/"), "one",
                           object_path("/Base/Path/one")),
           std::make_tuple(object_path("/Base/Path/"), "one/two",
                           object_path("/Base/Path/one/two")),
           std::make_tuple(object_path("/Base/Path/"), "one/two/foobar",
                           object_path("/Base/Path/one/two/foobar")),
           std::make_tuple(object_path("/Base/Path"), "",
                           object_path("/Base/Path/"))));

TEST_P(TestPathAppend, pathAppendsCorrectly)
{
    auto [basePath, extension, expectedPath] = GetParam();
    EXPECT_EQ(pathAppend(basePath, extension), expectedPath);
}

class TestPathAppendFail :
    public Test,
    public WithParamInterface<std::tuple<object_path, std::string>>
{};

INSTANTIATE_TEST_SUITE_P(
    _, TestPathAppendFail,
    Values(std::make_tuple(object_path("/Base/Path"), "/one"),
           std::make_tuple(object_path("/Base/Path"), "one/"),
           std::make_tuple(object_path("/Base/Path"), "one/two/"),
           std::make_tuple(object_path("/Base/Path"), "one//two"),
           std::make_tuple(object_path("/Base/Path"), "/"),
           std::make_tuple(object_path("/Base/Path"), "//")));

TEST_P(TestPathAppendFail, pathAppendsCorrectly)
{
    auto [basePath, extension] = GetParam();
    EXPECT_THROW(pathAppend(basePath, extension), errors::InvalidArgument);
}
