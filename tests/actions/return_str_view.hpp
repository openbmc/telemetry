#pragma once

#include <gmock/gmock.h>

namespace details
{

using namespace testing;

class ReturnStrView
{
  public:
    ReturnStrView(std::string_view ret) : ret_(ret)
    {}

    template <typename R, typename Tuple>
    R Perform(const Tuple& args) const
    {
        return ret_;
    }

  private:
    std::string ret_;
};

} // namespace details

testing::PolymorphicAction<details::ReturnStrView>
    ReturnStrView(std::string_view ret)
{
    return testing::MakePolymorphicAction(details::ReturnStrView(ret));
}
