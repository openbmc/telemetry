#pragma once

#include <concepts>
#include <variant>

namespace utils
{
namespace details
{

template <class T, class... Args>
auto removeMonostate(std::variant<T, Args...>) -> std::variant<T, Args...>;

template <class... Args>
auto removeMonostate(std::variant<std::monostate, Args...>)
    -> std::variant<Args...>;

template <class Variant>
struct WithoutMonostate
{
  private:
  public:
    using type = decltype(removeMonostate(Variant{}));
};

} // namespace details

template <class Variant>
using WithoutMonostate = typename details::WithoutMonostate<Variant>::type;

} // namespace utils
