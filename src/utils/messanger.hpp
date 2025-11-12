#pragma once

#include "messanger_service.hpp"

#include <boost/asio.hpp>

namespace utils
{

template <class Service>
class MessangerT
{
  public:
    explicit MessangerT(boost::asio::execution_context& execution_context) :
        service_(boost::asio::use_service<Service>(execution_context)),
        context_(service_.create())
    {}

    MessangerT(const MessangerT&) = delete;
    MessangerT& operator=(const MessangerT&) = delete;
    MessangerT(MessangerT&&) = delete;
    MessangerT& operator=(MessangerT&&) = delete;

    ~MessangerT()
    {
        service_.destroy(context_);
    }

    template <class EventType>
    void on_receive(std::function<void(const EventType&)> handler)
    {
        context_.handlers.emplace_back(std::move(handler));
    }

    template <class EventType>
    void send(const EventType& event)
    {
        service_.send(event);
    }

  private:
    Service& service_;
    typename Service::Context& context_;
};

using Messanger = MessangerT<MessangerService>;

} // namespace utils
