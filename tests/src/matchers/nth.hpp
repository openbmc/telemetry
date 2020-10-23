#pragma once

#include <gmock/gmock.h>

namespace detail
{

template <size_t N, class FieldType>
class NthMatcher
{
  public:
    template <class Matcher>
    NthMatcher(Matcher&& matcher) :
        matcher(testing::MatcherCast<FieldType>(std::move(matcher)))
    {}

    template <class T>
    bool MatchAndExplain(const T& tuple,
                         testing::MatchResultListener* listener) const
    {
        *listener << N << "th element is ";
        return testing::internal::MatchPrintAndExplain(std::get<N>(tuple),
                                                       matcher, listener);
    }

    void DescribeTo(std::ostream* os) const
    {
        *os << N << "th element is ";
        matcher.DescribeTo(os);
    }

    void DescribeNegationTo(std::ostream* os) const
    {
        *os << N << "th element is ";
        matcher.DescribeNegationTo(os);
    }

  private:
    testing::Matcher<FieldType> matcher;
};

} // namespace detail

template <size_t N, class TupleType, class Matcher>
inline auto Nth(Matcher&& matcher)
{
    using FieldType = std::tuple_element_t<N, TupleType>;

    return testing::MakePolymorphicMatcher(
        detail::NthMatcher<N, FieldType>(std::forward<Matcher>(matcher)));
}
