#pragma once

#include <entt/entt.hpp>

struct Transform {
    float x, y, z;
};

// template <> struct entt::component_traits<Transform> {
//     // Do not swap-and-pop when an element is erased – leave a tombstone instead.
//     using in_place_delete = std::true_type;
// };
