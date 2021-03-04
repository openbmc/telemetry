#pragma once

#include "types/collection_duration.hpp"

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

namespace detail
{

template <class U>
static U& ref();

template <class T>
struct has_utils_from_json
{
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

template <class T>
struct has_utils_to_json
{
    template <class U>
    static std::true_type
        check(decltype(utils::to_json(ref<nlohmann::json>(), ref<const U>()))*);

    template <class>
    static std::false_type check(...);

    static constexpr bool value =
        decltype(check<std::decay_t<T>>(nullptr))::value;
};

template <class T>
constexpr bool has_utils_to_json_v = has_utils_to_json<T>::value;

} // namespace detail

template <class, class...>
struct LabeledTuple;

template <class... Args, class... Labels>
struct LabeledTuple<std::tuple<Args...>, Labels...>
{
    static_assert(sizeof...(Args) == sizeof...(Labels));

    using tuple_type = std::tuple<Args...>;

    LabeledTuple() = default;
    LabeledTuple(const LabeledTuple&) = default;
    LabeledTuple(LabeledTuple&&) = default;

    LabeledTuple(tuple_type v) : value(std::move(v))
    {}
    LabeledTuple(Args... args) : value(std::move(args)...)
    {}

    LabeledTuple& operator=(const LabeledTuple&) = default;
    LabeledTuple& operator=(LabeledTuple&&) = default;

    nlohmann::json to_json() const
    {
        nlohmann::json j;
        to_json_all(j, std::make_index_sequence<sizeof...(Args)>());
        return j;
    }

    void from_json(const nlohmann::json& j)
    {
        from_json_all(j, std::make_index_sequence<sizeof...(Args)>());
    }

    template <size_t Idx>
    const auto& at_index() const
    {
        return std::get<Idx>(value);
    }

    template <size_t Idx>
    auto& at_index()
    {
        return std::get<Idx>(value);
    }

    template <class Label>
    const auto& at_label() const
    {
        return find_item<0, Label>(*this);
    }

    template <class Label>
    auto& at_label()
    {
        return find_item<0, Label>(*this);
    }

    bool operator==(const LabeledTuple& other) const
    {
        return value == other.value;
    }

    bool operator<(const LabeledTuple& other) const
    {
        return value < other.value;
    }

  private:
    template <size_t... Idx>
    void to_json_all(nlohmann::json& j, std::index_sequence<Idx...>) const
    {
        (to_json_item<Idx>(j), ...);
    }

    template <size_t Idx>
    void to_json_item(nlohmann::json& j) const
    {
        using Label = std::tuple_element_t<Idx, std::tuple<Labels...>>;
        using T = std::tuple_element_t<Idx, tuple_type>;
        nlohmann::json& item = j[Label::str()];
        if constexpr (detail::has_utils_to_json_v<T>)
        {
            utils::to_json(item, std::get<Idx>(value));
        }
        else
        {
            item = std::get<Idx>(value);
        }
    }

    template <size_t... Idx>
    void from_json_all(const nlohmann::json& j, std::index_sequence<Idx...>)
    {
        (from_json_item<Idx>(j), ...);
    }

    template <size_t Idx>
    void from_json_item(const nlohmann::json& j)
    {
        using Label = std::tuple_element_t<Idx, std::tuple<Labels...>>;
        using T = std::tuple_element_t<Idx, tuple_type>;
        const nlohmann::json& item = j.at(Label::str());
        if constexpr (detail::has_utils_from_json_v<T>)
        {
            T& v = std::get<Idx>(value);
            utils::from_json(item, v);
        }
        else
        {
            std::get<Idx>(value) = item.get<T>();
        }
    }

    template <size_t Idx, class Label, class Self>
    static auto& find_item(Self& self)
    {
        if constexpr (std::is_same_v<Label, std::tuple_element_t<
                                                Idx, std::tuple<Labels...>>>)
        {
            return std::get<Idx>(self.value);
        }
        else
        {
            return find_item<Idx + 1, Label>(self);
        }
    }

    tuple_type value;
};

template <class... Args, class... Labels>
inline void to_json(nlohmann::json& json,
                    const LabeledTuple<std::tuple<Args...>, Labels...>& tuple)
{
    json = tuple.to_json();
}

template <class... Args, class... Labels>
inline void from_json(const nlohmann::json& json,
                      LabeledTuple<std::tuple<Args...>, Labels...>& tuple)
{
    tuple.from_json(json);
}

} // namespace utils
