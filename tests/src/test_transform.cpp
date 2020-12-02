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
    EXPECT_TRUE(utils::detail::has_member_reserve_v<std::vector<std::string>>);
    ASSERT_THAT(output, ElementsAre("1", "2", "3"));
}

TEST(TestTransform, transformsSet)
{
    std::set<int> input = {1, 2, 3};
    std::set<std::string> output =
        utils::transform(input, [](int v) { return std::to_string(v); });
    EXPECT_FALSE(utils::detail::has_member_reserve_v<std::set<std::string>>);
    ASSERT_THAT(output, ElementsAre("1", "2", "3"));
}
