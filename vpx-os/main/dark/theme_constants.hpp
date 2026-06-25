#pragma once
#include "systems/base/esp_brookesia_base_context.hpp"

namespace esp_brookesia::systems::phone {

constexpr uint32_t COLOR_BG_LAUNCHER = 0x110000;
constexpr uint32_t COLOR_INACTIVE = 0x666666;
constexpr uint32_t COLOR_PRIMARY_ACCENT = 0xDD0000;
constexpr uint32_t COLOR_SECONDARY_ACCENT = 0xFFF000;
constexpr uint32_t COLOR_PURE_BLACK = 0x000000;
constexpr uint32_t COLOR_PURE_WHITE = 0xFFFFFF;

// TODO: probably better to have a variable for each UI element (clock, wifi,
// battery...). we'll get into that.

} // namespace esp_brookesia::systems::phone