#include "generate_unique_mock_id.hpp"

#include "helpers.hpp"

uint64_t generateUniqueMockId()
{
    static uint64_t id = 0u;
    return id++;
}
