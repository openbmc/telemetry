#include "messanger_service.hpp"

namespace utils
{

MessangerService::MessangerService(
    boost::asio::execution_context& execution_context) :
    boost::asio::execution_context::service(execution_context)
{}

MessangerService::Context& MessangerService::create()
{
    contexts_.emplace_back(std::make_unique<Context>());
    return *contexts_.back();
}

void MessangerService::destroy(MessangerService::Context& context)
{
    contexts_.erase(std::remove_if(contexts_.begin(), contexts_.end(),
                                   [&context](const auto& item) {
        return item.get() == &context;
    }),
                    contexts_.end());
}

boost::asio::execution_context::id MessangerService::id = {};

} // namespace utils
