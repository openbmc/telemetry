#pragma once

#include <nlohmann/json.hpp>

#include <tuple>

namespace utils
{

template <class, class...>
struct TupleWrapper;

template <class... Args, class... Labels>
struct TupleWrapper<std::tuple<Args...>, Labels...>
{
    static_assert(sizeof...(Args) == sizeof...(Labels));

    TupleWrapper(const std::tuple<Args...>& tuple) : o(&tuple)
    {}

    nlohmann::json to_json() const
    {
        return to_json_all(std::make_index_sequence<sizeof...(Args)>());
    }

  private:
    template <size_t... Idx>
    nlohmann::json to_json_all(std::index_sequence<Idx...>) const
    {
        nlohmann::json result;
        (to_json_item<Idx>(result), ...);
        return result;
    }

    template <size_t Idx>
    void to_json_item(nlohmann::json& result) const
    {
        using Label = std::tuple_element_t<Idx, std::tuple<Labels...>>;
        result[Label::str()] = std::get<Idx>(*o);
    }

    const std::tuple<Args...>* o;
};

template <class... Args, class... Labels>
void to_json(nlohmann::json& item,
             const TupleWrapper<std::tuple<Args...>, Labels...>& wrapper)
{
    item = wrapper.to_json();
}

} // namespace utils
