#pragma once

#include <array>
#include <string_view>

namespace sha1 {

using Hash = std::array<unsigned char, 20>;

[[nodiscard]] auto hash(std::string_view view) -> Hash;

};
