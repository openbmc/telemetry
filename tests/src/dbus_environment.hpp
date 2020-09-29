#include <sdbusplus/asio/object_server.hpp>

#include <future>
#include <thread>

#include <gmock/gmock.h>

class DbusEnvironment : public ::testing::Environment
{
  public:
    void SetUp() override;
    void TearDown() override;

    static boost::asio::io_context& getIoc();
    static std::shared_ptr<sdbusplus::asio::connection> getBus();
    static std::shared_ptr<sdbusplus::asio::object_server> getObjServer();
    static const char* serviceName();
    static void synchronizeIoc(
        std::chrono::milliseconds timeout = std::chrono::seconds(5));
    static std::function<void()> setPromise(std::string_view name);
    static void waitForFuture(
        std::string_view name,
        std::chrono::milliseconds timeout = std::chrono::seconds(5));

  private:
    static boost::asio::io_context ioc;
    static std::shared_ptr<sdbusplus::asio::connection> bus;
    static std::shared_ptr<sdbusplus::asio::object_server> objServer;
    static std::mutex mutex;
    static std::map<std::string, std::vector<std::future<bool>>> futures;

    std::thread iocThread;

    static std::future<bool> getFuture(std::string_view name);
};
