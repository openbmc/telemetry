#include "dbus/dbus.hpp"
#include "utils/utils.hpp"

#include <gmock/gmock.h>

using namespace testing;

TEST(DBusTest, subObject)
{
    EXPECT_THAT(dbus::subObject("name"),
                std::string(dbus::Service) + std::string(".name"));
}

TEST(DBusTest, subIface)
{
    EXPECT_THAT(dbus::subIface("name"),
                std::string(dbus::Iface) + std::string(".name"));
}

TEST(DBusTest, subPath)
{
    EXPECT_THAT(dbus::subPath("name"),
                std::string(dbus::Path) + std::string("/name"));
}

TEST(DBusTest, throwErrorThrowsDbbusplusException)
{
    EXPECT_THROW(utils::throwError(std::errc::invalid_argument, "message"),
                 sdbusplus::exception::SdBusError);
}

TEST(DBusTest, convertsStringToEnum)
{
    EXPECT_THAT(dbus::toEnum(core::reportingTypeConvertData, "OnChange"),
                Eq(core::ReportingType::onChange));
    EXPECT_THAT(dbus::toEnum(core::reportingTypeConvertData, "OnRequest"),
                Eq(core::ReportingType::onRequest));
}

TEST(DBusTest, convertsStringToEnumThrowsWhenStringValueIsNotMatched)
{
    using namespace std::string_literals;

    EXPECT_ANY_THROW(
        dbus::toEnum(core::reportingTypeConvertData, "IncorrectString"s));
    EXPECT_ANY_THROW(
        dbus::toEnum(core::reportingTypeConvertData, {"IncorrectString"s}));
}

TEST(DBusTest, convertsVectorOfStringsToVectorOfEnums)
{
    using namespace std::string_literals;

    EXPECT_THAT(dbus::toEnum(core::reportingTypeConvertData,
                             {"OnChange"s, "OnRequest"s}),
                ElementsAre(core::ReportingType::onChange,
                            core::ReportingType::onRequest));
}

TEST(DBusTest, convertsEnumToString)
{
    EXPECT_THAT(dbus::toString(core::reportingTypeConvertData,
                               core::ReportingType::onChange),
                Eq("OnChange"));
    EXPECT_THAT(dbus::toString(core::reportingTypeConvertData,
                               core::ReportingType::onRequest),
                Eq("OnRequest"));
}

TEST(DBusTest, convertsEnumToStringThrowsWhenEnumValueIsNotMatched)
{
    using Type = std::underlying_type_t<core::ReportingType>;
    const auto incorrectEnumValue =
        static_cast<core::ReportingType>(std::numeric_limits<Type>::max());

    EXPECT_ANY_THROW(
        dbus::toString(core::reportingTypeConvertData, incorrectEnumValue));
    EXPECT_ANY_THROW(
        dbus::toString(core::reportingTypeConvertData, {incorrectEnumValue}));
}

TEST(DBusTest, convertsVectorOfEnumsToVectorOfStrings)
{
    EXPECT_THAT(dbus::toString(core::reportingTypeConvertData,
                               {core::ReportingType::onChange,
                                core::ReportingType::onRequest}),
                ElementsAre("OnChange", "OnRequest"));
}
