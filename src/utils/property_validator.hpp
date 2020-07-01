#pragma once

#include <functional>

namespace utils
{
template <class T>
class PropertyValidator
{
  public:
    virtual ~PropertyValidator() = default;

    template <class F>
    void addValidator(F&& validator)
    {
        validators_.emplace_back(std::forward<F>(validator));
    }

  protected:
    bool validate(const T& value) const
    {
        return std::all_of(
            std::begin(validators_), std::end(validators_),
            [&value](const auto& validator) { return validator(value); });
    }

  private:
    std::vector<std::function<bool(const T&)>> validators_;
};
} // namespace utils
