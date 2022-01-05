#pragma once
#include <string_view>

class Error {
public:
	Error(const char* str);
	auto string() const -> std::string_view;

	operator bool();

	friend auto operator<<(std::ostream& os, const Error err) -> std::ostream&;
	static auto check(Error err, const char* file, int line) -> void;
private:
	std::string_view error;
};

#define validate(err) Error::check(err, __FILE__, __LINE__)
