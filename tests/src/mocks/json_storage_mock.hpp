#pragma once

#include "interfaces/json_storage.hpp"

#include <gmock/gmock.h>

class StorageMock : public interfaces::JsonStorage
{
  public:
    MOCK_METHOD(void, store, (const FilePath&, const nlohmann::json&),
                (override));
    MOCK_METHOD(bool, remove, (const FilePath&), (override));
    MOCK_METHOD(bool, isExist, (const FilePath&), (const, override));
    MOCK_METHOD(std::optional<nlohmann::json>, load, (const FilePath&),
                (const, override));
    MOCK_METHOD(std::vector<FilePath>, list, (), (const, override));
};
