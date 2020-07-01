#pragma once

#include <string>

namespace core
{

class ReportName
{
  public:
    ReportName(const std::string& domain, const std::string& name) :
        domain_(domain), name_(name)
    {}

    std::string str() const
    {
        return domain_ + "/" + name_;
    }

    std::string_view name() const
    {
        return name_;
    }

    std::string_view domain() const
    {
        return domain_;
    }

    bool operator==(const ReportName& other) const
    {
        return std::tie(domain_, name_) == std::tie(other.domain_, other.name_);
    }

    friend std::ostream& operator<<(std::ostream& os, const ReportName& o)
    {
        os << o.domain_ << "/" << o.name_;
        return os;
    }

  private:
    std::string domain_;
    std::string name_;
};

} // namespace core
