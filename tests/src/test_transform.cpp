#include "helpers.hpp"
#include "utils/transform.hpp"

#include <set>
#include <vector>

#include <gmock/gmock.h>

using namespace testing;

TEST(TestTransform, transformsVector)
{
    std::vector<int> input = {1, 2, 3};
    std::vector<std::string> output =
        utils::transform(input, [](int v) { return std::to_string(v); });
    EXPECT_TRUE(utils::detail::has_member_reserve<decltype(input)>);
    EXPECT_TRUE(utils::detail::has_member_reserve<decltype(output)>);
    ASSERT_THAT(output, ElementsAre("1", "2", "3"));
}

TEST(TestTransform, transformsSet)
{
    std::set<int> input = {1, 2, 3};
    std::set<std::string> output =
        utils::transform(input, [](int v) { return std::to_string(v); });
    EXPECT_FALSE(utils::detail::has_member_reserve<decltype(input)>);
    EXPECT_FALSE(utils::detail::has_member_reserve<decltype(output)>);
    ASSERT_THAT(output, ElementsAre("1", "2", "3"));
}

TEST(TestTransform, transformsArrayToVector)
{
    std::array<int, 3> input = {1, 2, 3};
    std::vector<std::string> output =
        utils::transform<std::vector<std::string>>(
            input, [](int v) { return std::to_string(v); });
    EXPECT_FALSE(utils::detail::has_member_reserve<decltype(input)>);
    EXPECT_TRUE(utils::detail::has_member_reserve<decltype(output)>);
    ASSERT_THAT(output, ElementsAre("1", "2", "3"));
}
