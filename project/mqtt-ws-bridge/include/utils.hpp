#pragma once
#include <string>
#include <string_view>

using Byte = unsigned char;

auto consumeFile(const char* path) -> std::string;

auto getPath(std::string_view path) -> std::string_view;

auto endsWith(std::string_view view, std::string_view ending) -> bool;
