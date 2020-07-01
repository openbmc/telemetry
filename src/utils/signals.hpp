#pragma once

#include <functional>
#include <memory>

namespace utils::signals
{
namespace detail
{

class Connection
{
  public:
    virtual ~Connection() = default;
};

template <class Signature>
class ConnectionT : public Connection
{
  public:
    explicit ConnectionT(std::shared_ptr<std::function<Signature>> callback) :
        callback_(std::move(callback))
    {}

  private:
    std::shared_ptr<std::function<Signature>> callback_;
};

} // namespace detail

class Connection
{
  public:
    Connection() = default;
    explicit Connection(std::unique_ptr<detail::Connection> connection) :
        connection_(std::move(connection))
    {}

  private:
    std::unique_ptr<detail::Connection> connection_;
};

template <class Signature>
class Signal
{
  public:
    template <class... Args>
    void operator()(Args&&... args) const
    {
        for (const auto& weakCallback : callbacks_)
        {
            if (auto callback = weakCallback.lock())
            {
                (*callback)(std::forward<Args>(args)...);
            }
        }
    }

    template <class F>
    Connection connect(F&& fun)
    {
        auto callback =
            std::make_shared<std::function<Signature>>(std::forward<F>(fun));
        callbacks_.emplace_back(callback);

        return Connection(std::make_unique<detail::ConnectionT<Signature>>(
            std::move(callback)));
    }

  private:
    std::vector<std::weak_ptr<std::function<Signature>>> callbacks_;
};

} // namespace utils::signals
