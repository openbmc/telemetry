#pragma once

#include "interfaces/json_storage.hpp"

#include <gmock/gmock.h>

class StorageMock : public interfaces::JsonStorage
{
  public:
    ~StorageMock();

    MOCK_METHOD2(store, void(const FileResource&, const nlohmann::json&));
    MOCK_METHOD1(remove, bool(const FileResource&));
    MOCK_CONST_METHOD1(load,
                       std::optional<nlohmann::json>(const FileResource&));
    MOCK_CONST_METHOD1(list,
                       std::vector<FileResource>(const DirectoryResource&));
};
