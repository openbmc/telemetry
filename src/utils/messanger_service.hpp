#pragma once

#include <boost/asio.hpp>

#include <any>

namespace utils
{

class MessangerService : public boost::asio::execution_context::service
{
  public:
    using key_type = MessangerService;

    struct Context
    {
        std::vector<std::any> handlers;
    };

    MessangerService(boost::asio::execution_context& execution_context);
    ~MessangerService() = default;

    void shutdown() {}

    Context& create();
    void destroy(Context& context);

    template <class T>
    void send(const T& event)
    {
        using HandlerType = std::function<void(const T&)>;

        for (const auto& context : contexts_)
        {
            for (const auto& any : context->handlers)
            {
                if (const HandlerType* handler =
                        std::any_cast<HandlerType>(&any))
                {
                    (*handler)(event);
                }
            }
        }
    }

    static boost::asio::execution_context::id id;

  private:
    std::vector<std::unique_ptr<Context>> contexts_;
};

} // namespace utils
