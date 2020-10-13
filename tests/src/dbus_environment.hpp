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
    static std::function<void()> setPromise(std::string_view name);
    static void sleepFor(std::chrono::milliseconds);

    static void synchronizeIoc()
    {
        while (ioc.poll() > 0)
        {
        }
    }

    template <class Functor>
    static void synchronizedPost(Functor&& functor)
    {
        boost::asio::post(ioc, std::forward<Functor>(functor));
        synchronizeIoc();
    }

    template <class T>
    static std::optional<T> waitForFuture(
        std::future<T> future,
        std::chrono::milliseconds timeout = std::chrono::seconds(10))
    {
        constexpr auto precission = std::chrono::milliseconds(10);
        auto elapsed = std::chrono::milliseconds(0);

        while (future.valid() && elapsed < timeout)
        {
            synchronizeIoc();

            try
            {
                if (future.wait_for(precission) == std::future_status::ready)
                {
                    return future.get();
                }
                else
                {
                    elapsed += precission;
                }
            }
            catch (const std::future_error& e)
            {
                std::cerr << e.what() << "\n";
                return {};
            }
        }

        return {};
    }

    static bool waitForFuture(
        std::string_view name,
        std::chrono::milliseconds timeout = std::chrono::seconds(10));

  private:
    static std::future<bool> getFuture(std::string_view name);

    static boost::asio::io_context ioc;
    static std::shared_ptr<sdbusplus::asio::connection> bus;
    static std::shared_ptr<sdbusplus::asio::object_server> objServer;
    static std::map<std::string, std::vector<std::future<bool>>> futures;
    static bool setUp;
};
