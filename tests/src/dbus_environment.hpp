#include "telemetry/names.hpp"
#include "telemetry/types.hpp"

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

    struct AddReportArgs
    {
        std::string name;
        std::string reportingType;
        bool emitsReadingsSignal;
        bool logToMetricReportsCollection;
        uint64_t interval;
        ReadingParameters readingParams;

        inline std::string getReportPath() const
        {
            return reportDir + name;
        }
    };

    static std::pair<boost::system::error_code, std::string>
        addReport(const AddReportArgs& args);
    static boost::system::error_code deleteReport(const std::string& path);

    template <class T>
    static T getProperty(const std::string& path, const std::string& iface,
                         const std::string& property)
    {
        std::promise<T> propertyPromise;
        sdbusplus::asio::getProperty<T>(
            *getBus(), serviceName(), path, iface, property,
            [&propertyPromise](boost::system::error_code ec) {
                EXPECT_THAT(static_cast<bool>(ec), ::testing::Eq(false));
                propertyPromise.set_value(T{});
            },
            [&propertyPromise](T t) { propertyPromise.set_value(t); });
        return propertyPromise.get_future().get();
    }

    template <class T>
    static T getReportManagerProperty(const std::string& property)
    {
        return getProperty<T>(reportManagerPath, reportManagerIfaceName,
                              property);
    }

    template <class T>
    static T getReportProperty(const std::string& path,
                               const std::string& property)
    {
        return getProperty<T>(path, reportIfaceName, property);
    }

    template <class T>
    static boost::system::error_code
        setReportProperty(const std::string& path, const std::string& property,
                          const T& newValue)
    {
        std::promise<boost::system::error_code> setPromise;
        sdbusplus::asio::setProperty(
            *getBus(), serviceName(), path, reportIfaceName, property,
            std::move(newValue),
            [&setPromise](boost::system::error_code ec) {
                setPromise.set_value(ec);
            },
            [&setPromise]() {
                setPromise.set_value(boost::system::error_code{});
            });
        return setPromise.get_future().get();
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
