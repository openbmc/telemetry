#include "metric.hpp"

#include <gmock/gmock.h>

using namespace testing;

class TestMetric : public Test
{
  public:
    Metric sut = {};
};