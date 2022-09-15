#pragma once

#include <nlohmann/json.hpp>
#include <sdbusplus/message/types.hpp>

#include <cmath>
#include <limits>

namespace utils
{

namespace numeric_literals
{
constexpr std::string_view NaN = "NaN";
constexpr std::string_view infinity = "inf";
constexpr std::string_view infinity_negative = "-inf";
} // namespace numeric_literals

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

inline void to_json(nlohmann::json& j, const double& val)
{
    if (std::isnan(val))
    {
        j = numeric_literals::NaN;
    }
    else if (val == std::numeric_limits<double>::infinity())
    {
        j = numeric_literals::infinity;
    }
    else if (val == -std::numeric_limits<double>::infinity())
    {
        j = numeric_literals::infinity_negative;
    }
    else
    {
        j = val;
    }
}

inline void from_json(const nlohmann::json& j, double& val)
{
    if (j.is_number())
    {
        val = j.get<double>();
    }
    else
    {
        auto str_val = j.get<std::string>();
        if (str_val == numeric_literals::NaN)
        {
            val = std::numeric_limits<double>::quiet_NaN();
        }
        else if (str_val == numeric_literals::infinity)
        {
            val = std::numeric_limits<double>::infinity();
        }
        else if (str_val == numeric_literals::infinity_negative)
        {
            val = -std::numeric_limits<double>::infinity();
        }
        else
        {
            throw std::invalid_argument("Unknown numeric literal");
        }
    }
}

namespace detail
{

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

template <class T>
struct has_utils_to_json
{
    template <class U>
    static U& ref();

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

bool eq(const auto& a, const auto& b)
{
    if constexpr (std::is_same<std::decay_t<decltype(a)>, double>())
    {
        if (std::isnan(a))
        {
            return std::isnan(b);
        }
    }
    return a == b;
}

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

    explicit LabeledTuple(tuple_type v) : value(std::move(v))
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

    const tuple_type& to_tuple() const
    {
        return value;
    }

    void from_json(const nlohmann::json& j)
    {
        from_json_all(j, std::make_index_sequence<sizeof...(Args)>());
    }

    std::string dump() const
    {
        return to_json().dump();
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
        bool result = true;
        std::apply(
            [&](auto&&... x) {
                return std::apply(
                    [&](auto&&... y) { ((result &= detail::eq(x, y)), ...); },
                    value);
            },
            other.value);
        return result;
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
            static_assert(Idx + 1 < sizeof...(Args),
                          "Label not found in LabeledTuple");
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
