#pragma once

#include "core/interfaces/json_storage.hpp"

#include <gmock/gmock.h>

namespace core
{

class JsonStorageMock : public interfaces::JsonStorage
{
  public:
    MOCK_METHOD2(store, void(const Resource&, const nlohmann::json&));
    MOCK_METHOD1(remove, bool(const Resource&));
    MOCK_CONST_METHOD1(load, std::optional<nlohmann::json>(const Resource&));
    MOCK_CONST_METHOD1(
        list,
        std::vector<core::interfaces::JsonStorage::Resource>(const Resource&));
};

} // namespace core
