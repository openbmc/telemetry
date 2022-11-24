#include "helpers.hpp"
#include "utils/labeled_tuple.hpp"

#include <limits>

#include <gmock/gmock.h>

using namespace testing;

struct TestingLabelDouble
{
    static std::string str()
    {
        return "DoubleValue";
    }
};

struct TestingLabelString
{
    static std::string str()
    {
        return "StringValue";
    }
};

using LabeledTestingTuple =
    utils::LabeledTuple<std::tuple<double, std::string>, TestingLabelDouble,
                        TestingLabelString>;

class TestLabeledTupleDoubleSpecialization :
    public Test,
    public WithParamInterface<
        std::tuple<double, std::variant<double, std::string>>>
{
  public:
    const std::string string_value = "Some value";
};

TEST_P(TestLabeledTupleDoubleSpecialization,
       serializeAndDeserializeMakesSameTuple)
{
    auto [double_value, expected_serialized_value] = GetParam();
    LabeledTestingTuple initial(double_value, string_value);
    nlohmann::json serialized(initial);

    EXPECT_EQ(serialized["StringValue"], string_value);

    auto& actual_serialized_value = serialized["DoubleValue"];
    if (std::holds_alternative<std::string>(expected_serialized_value))
    {
        EXPECT_TRUE(actual_serialized_value.is_string());
        EXPECT_EQ(actual_serialized_value.get<std::string>(),
                  std::get<std::string>(expected_serialized_value));
    }
    else
    {
        EXPECT_TRUE(actual_serialized_value.is_number());
        EXPECT_EQ(actual_serialized_value.get<double>(),
                  std::get<double>(expected_serialized_value));
    }

    LabeledTestingTuple deserialized = serialized.get<LabeledTestingTuple>();
    EXPECT_EQ(initial, deserialized);
}

INSTANTIATE_TEST_SUITE_P(
    _, TestLabeledTupleDoubleSpecialization,
    Values(std::make_tuple(10.0, std::variant<double, std::string>(10.0)),
           std::make_tuple(std::numeric_limits<double>::infinity(),
                           std::variant<double, std::string>("inf")),
           std::make_tuple(-std::numeric_limits<double>::infinity(),
                           std::variant<double, std::string>("-inf")),
           std::make_tuple(std::numeric_limits<double>::quiet_NaN(),
                           std::variant<double, std::string>("NaN"))));

TEST(TestLabeledTupleDoubleSpecializationNegative,
     ThrowsWhenUnknownLiteralDuringDeserialization)
{
    nlohmann::json data = nlohmann::json{{"DoubleValue", "FooBar"},
                                         {"StringValue", "Some Text Val"}};

    EXPECT_THROW(data.get<LabeledTestingTuple>(), std::invalid_argument);
}
