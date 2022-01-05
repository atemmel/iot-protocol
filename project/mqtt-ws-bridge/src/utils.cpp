#include "utils.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>

auto consumeFile(const char* path) -> std::string {
	std::ifstream file;
	file.open(path, std::ios::in | std::ios::binary | std::ios::ate);
	
	auto size = file.tellg();
	file.seekg(0, std::ios::beg);

	if(size < 1) {
		return std::string();
	}

	std::vector<char> bytes(size);
	file.read(bytes.data(), size);

	return std::string(bytes.data(), size);
}

auto getPath(std::string_view path) -> std::string_view {
	auto index = path.rfind(std::filesystem::path::preferred_separator);
	return index == std::string_view::npos ? path : path.substr(0, index);
}

auto endsWith(std::string_view sv, std::string_view end) -> bool {
	if(end.size() > sv.size() ) return false;
	return std::equal(end.rbegin(), end.rend(), sv.rbegin() );
}
