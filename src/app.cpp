#include "app.hpp"

App::App(std::shared_ptr<sdbusplus::asio::connection> bus) :
    objServer(std::make_shared<sdbusplus::asio::object_server>(bus)),
    reportManager(objServer)
{}
