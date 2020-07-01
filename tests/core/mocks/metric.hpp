#pragma once

#include "core/interfaces/metric.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

#include <chrono>
#include <thread>

#include <gmock/gmock.h>

namespace core
{

using namespace testing;

class MetricMock : public interfaces::Metric
{
  public:
    MetricMock(boost::asio::io_context& ioc) : ioc_(ioc)
    {
        on_call_async_read(std::chrono::milliseconds());

        ON_CALL(*this, readings()).WillByDefault(Invoke([this] {
            return boost::beast::span<const std::optional<Reading>>(
                reading_to_return.data(), reading_to_return.size());
        }));
    }

    ~MetricMock()
    {
        synchronize();
    }

    bool operator==(const interfaces::Metric& other) const override final
    {
        return this == &other;
    }

    MOCK_CONST_METHOD0(type, OperationType());
    MOCK_CONST_METHOD0(id, std::string_view());
    MOCK_CONST_METHOD0(metadata, std::string_view());
    MOCK_CONST_METHOD0(
        sensors, const std::vector<std::shared_ptr<interfaces::Sensor>>&());
    MOCK_CONST_METHOD0(readings,
                       boost::beast::span<const std::optional<Reading>>());
    MOCK_METHOD1(async_read, void(ReadingsUpdatedCallback&&));
    MOCK_METHOD0(registerForUpdates, void());

    void on_call_async_read(std::chrono::milliseconds delay)
    {
        ON_CALL(*this, async_read(_))
            .WillByDefault(Invoke([this, delay](const auto& callback) {
                synchronize();

                if (delay == std::chrono::milliseconds())
                {
                    boost::asio::post(ioc_, callback);
                }
                else
                {
                    thread_ = std::thread([this, delay, callback] {
                        std::this_thread::sleep_for(delay);

                        boost::asio::post(ioc_, callback);
                    });
                }
            }));
    }

    void synchronize()
    {
        if (thread_.joinable())
        {
            thread_.join();
        }
    }

    std::vector<std::optional<Reading>> reading_to_return;

  private:
    std::thread thread_;
    boost::asio::io_context& ioc_;
};

} // namespace core
