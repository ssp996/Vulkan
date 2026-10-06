#include "enums.hpp"

bool operator<(ResourceTypes a, ResourceTypes b)
{
    return static_cast<uint32_t>(a) < static_cast<uint32_t>(b);
}