#pragma once

#include "interfaces/json_storage.hpp"

#include <gmock/gmock.h>

class StorageMock : public interfaces::JsonStorage
{
  public:
    MOCK_METHOD2(store, void(const FilePath&, const nlohmann::json&));
    MOCK_METHOD1(remove, bool(const FilePath&));
    MOCK_CONST_METHOD1(load, std::optional<nlohmann::json>(const FilePath&));
    MOCK_CONST_METHOD1(list, std::vector<FilePath>(const DirectoryPath&));
};
