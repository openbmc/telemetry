#pragma once

#include <boost/beast/core/span.hpp>

#include <type_traits>

#include <gmock/gmock.h>

namespace details
{

using namespace testing;

template <class T>
class ReturnSpan
{
  public:
    ReturnSpan(std::vector<std::remove_cv_t<T>> ret) : ret_(std::move(ret))
    {}

    template <typename R, typename Tuple>
    R Perform(const Tuple& args) const
    {
        return boost::beast::span<const T>(ret_.data(), ret_.size());
    }

  private:
    std::vector<std::remove_cv_t<T>> ret_;
};

} // namespace details

template <class R = void, class T>
auto ReturnSpan(std::vector<T> ret)
{
    using Conv = std::conditional_t<std::is_same_v<R, void>, T, R>;

    if constexpr (std::is_same_v<T, Conv>)
    {
        return testing::MakePolymorphicAction(
            details::ReturnSpan<Conv>(std::move(ret)));
    }
    else
    {
        std::vector<std::remove_cv_t<Conv>> conv;
        std::transform(std::begin(ret), std::end(ret), std::back_inserter(conv),
                       [](const T& item) { return item; });
        return testing::MakePolymorphicAction(
            details::ReturnSpan<Conv>(std::move(conv)));
    }
}
