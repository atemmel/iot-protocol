#pragma once

#include <string>
#include <string_view>

namespace base64 {

[[nodiscard]] auto encode(std::string_view view) -> std::string;

};
