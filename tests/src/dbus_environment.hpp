#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/property.hpp>

#include <future>
#include <thread>

#include <gmock/gmock.h>

class DbusEnvironment : public ::testing::Environment
{
  public:
    ~DbusEnvironment();

    void SetUp() override;
    void TearDown() override;
    void teardown();

    static boost::asio::io_context& getIoc();
    static std::shared_ptr<sdbusplus::asio::connection> getBus();
    static std::shared_ptr<sdbusplus::asio::object_server> getObjServer();
    static const char* serviceName();
    static bool synchronizeIoc(
        std::chrono::milliseconds timeout = std::chrono::seconds(5));
    static std::function<void()> setPromise(std::string_view name);
    static bool waitForFuture(
        std::string_view name,
        std::chrono::milliseconds timeout = std::chrono::seconds(5));

    template <class Functor>
    static bool synchronizedPost(
        Functor&& functor,
        std::chrono::milliseconds timeout = std::chrono::seconds(5))
    {
        auto promise = std::make_unique<std::promise<bool>>();
        auto future = promise->get_future();

        boost::asio::post(
            ioc, [promise = std::move(promise), functor = std::move(functor)] {
                try
                {
                    functor();
                    promise->set_value(true);
                }
                catch (const std::exception& e)
                {
                    promise->set_value(false);
                }
            });

        if (future.wait_for(timeout) == std::future_status::ready)
        {
            return future.get();
        }

        return false;
    }

  private:
    static std::future<bool> getFuture(std::string_view name);

    static boost::asio::io_context ioc;
    static std::shared_ptr<sdbusplus::asio::connection> bus;
    static std::shared_ptr<sdbusplus::asio::object_server> objServer;
    static std::mutex mutex;
    static std::map<std::string, std::vector<std::future<bool>>> futures;

    bool setUp = false;
    std::thread iocThread;
};
