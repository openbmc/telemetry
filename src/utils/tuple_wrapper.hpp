#pragma once

#include <nlohmann/json.hpp>
#include <sdbusplus/message/types.hpp>

namespace utils
{

inline void from_json(const nlohmann::json& j,
                      sdbusplus::message::object_path& o)
{
    o = j.get<std::string>();
}

inline void from_json(const nlohmann::json& j,
                      std::vector<sdbusplus::message::object_path>& o)
{
    o.clear();
    for (const nlohmann::json& item : j)
    {
        o.emplace_back(item.get<std::string>());
    }
}

template <class T>
struct has_utils_from_json
{
    template <class U>
    static U& ref();

    template <class U>
    static std::true_type check(
        decltype(utils::from_json(ref<const nlohmann::json>(), ref<U>()))*);

    template <class>
    static std::false_type check(...);

    static constexpr bool value =
        decltype(check<std::decay_t<T>>(nullptr))::value;
};

template <class T>
constexpr bool has_utils_from_json_v = has_utils_from_json<T>::value;

template <class, class...>
struct TupleWrapper;

template <class... Args, class... Labels>
struct TupleWrapper<std::tuple<Args...>, Labels...>
{
    static_assert(sizeof...(Args) == sizeof...(Labels));

    static nlohmann::json to_json(const std::tuple<Args...>& tuple)
    {
        nlohmann::json j;
        to_json_all(j, tuple, std::make_index_sequence<sizeof...(Args)>());
        return j;
    }

    static std::tuple<Args...> from_json(const nlohmann::json& j)
    {
        std::tuple<Args...> value;
        from_json_all(j, value, std::make_index_sequence<sizeof...(Args)>());
        return value;
    }

  private:
    template <size_t... Idx>
    static void to_json_all(nlohmann::json& j, const std::tuple<Args...>& tuple,
                            std::index_sequence<Idx...>)
    {
        (to_json_item<Idx>(j, tuple), ...);
    }

    template <size_t Idx>
    static void to_json_item(nlohmann::json& j,
                             const std::tuple<Args...>& tuple)
    {
        using Label = std::tuple_element_t<Idx, std::tuple<Labels...>>;
        j[Label::str()] = std::get<Idx>(tuple);
    }

    template <size_t... Idx>
    static void from_json_all(const nlohmann::json& j,
                              std::tuple<Args...>& value,
                              std::index_sequence<Idx...>)
    {
        (from_json_item<Idx>(j, value), ...);
    }

    template <size_t Idx>
    static void from_json_item(const nlohmann::json& j,
                               std::tuple<Args...>& value)
    {
        using Label = std::tuple_element_t<Idx, std::tuple<Labels...>>;
        using T = std::tuple_element_t<Idx, std::tuple<Args...>>;
        const nlohmann::json& item = j.at(Label::str());
        if constexpr (has_utils_from_json_v<T>)
        {
            T& v = std::get<Idx>(value);
            utils::from_json(item, v);
        }
        else
        {
            std::get<Idx>(value) = item.get<T>();
        }
    }

    const std::tuple<Args...>* o;
};

} // namespace utils
