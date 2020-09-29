#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>

#include <chrono>

namespace utils
{

template <class F>
void makeDetachedTimer(boost::asio::io_context& ioc,
                       std::chrono::milliseconds delay, F&& fun)
{
    auto timer = std::make_unique<boost::asio::steady_timer>(ioc);
    timer->expires_from_now(delay);
    timer->async_wait([timer = std::move(timer),
                       fun = std::move(fun)](boost::system::error_code ec) {
        if (ec)
        {
            return;
        }
        fun();
    });
}

} // namespace utils
