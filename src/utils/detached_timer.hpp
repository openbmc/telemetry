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
        ioc_(ioc),
        delay_(delay), fun_(std::move(fun))
    {}
    DetachedTimer(const DetachedTimer&) = delete;

    DetachedTimer& operator==(const DetachedTimer&) = delete;

    void run(boost::asio::yield_context& yield)
    {
        timer_.expires_from_now(
            boost::posix_time::milliseconds(delay_.count()));
        timer_.async_wait(
            [job = this->shared_from_this()](
                const boost::system::error_code& e) { job->fun_(); });
    }

  private:
    boost::asio::io_context& ioc_;
    std::chrono::milliseconds delay_;
    boost::asio::deadline_timer timer_{ioc_};
    F fun_;
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
