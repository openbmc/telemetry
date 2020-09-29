#pragma once

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>

#include <chrono>

namespace utils
{

template <class F>
struct DetachedTimer : public std::enable_shared_from_this<DetachedTimer<F>>
{
    DetachedTimer(boost::asio::io_context& ioc, std::chrono::milliseconds delay,
                  F&& fun) :
        ioc(ioc),
        delay(delay), fun(std::move(fun))
    {}

    DetachedTimer(const DetachedTimer&) = delete;
    DetachedTimer& operator=(const DetachedTimer&) = delete;

    void run(boost::asio::yield_context& yield)
    {
        timer.expires_from_now(boost::posix_time::milliseconds(delay.count()));
        timer.async_wait(
            [job = this->shared_from_this()](
                const boost::system::error_code& e) { job->fun(); });
    }

  private:
    boost::asio::io_context& ioc;
    std::chrono::milliseconds delay;
    boost::asio::deadline_timer timer{ioc};
    F fun;
};

template <class F>
void makeDetachedTimer(boost::asio::io_context& ioc,
                       std::chrono::milliseconds delay, F&& fun)
{
    auto job =
        std::make_shared<DetachedTimer<F>>(ioc, delay, std::forward<F>(fun));

    boost::asio::spawn(
        ioc, [job](boost::asio::yield_context yield) { job->run(yield); });
}

} // namespace utils
