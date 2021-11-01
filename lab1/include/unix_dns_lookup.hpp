#pragma once
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "common.hpp"
#include "error.hpp"

auto dnsLookup(std::string_view address, uint16_t port) -> std::tuple<std::vector<std::string>, Error>;
