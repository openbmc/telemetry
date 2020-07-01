#pragma once

#include "core/interfaces/json_storage.hpp"

#include <gmock/gmock.h>

namespace core
{

class JsonStorageMock : public interfaces::JsonStorage
{
  public:
    MOCK_METHOD2(store,
                 void(const std::filesystem::path&, const nlohmann::json&));
    MOCK_METHOD1(remove, bool(const std::filesystem::path&));
    MOCK_CONST_METHOD1(
        load, std::optional<nlohmann::json>(const std::filesystem::path&));
    MOCK_CONST_METHOD1(
        list, std::vector<std::filesystem::path>(const std::filesystem::path&));
};

} // namespace core
