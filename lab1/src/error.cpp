#include "error.hpp"

#include <iostream>

static constexpr const char* const findLastSlash(const char* const str, 
		const char* const lastSlash)
{
    return
		// If terminator found, return position of last slash
        *str == '\0' ? lastSlash :
		// Otherwise, keep last slash as one char beyond last slash
        *str == '/'  ? findLastSlash(str + 1, str + 1) :
				// If slash not found, keep old position of slash
                       findLastSlash(str + 1, lastSlash);
}

static constexpr const char* const findLastSlash(const char* const str) 
{ 
    return findLastSlash(str, str);
}

Error::Error(const char* str) : error(str == nullptr ? "" : str) {};

auto Error::string() const -> std::string_view {
	return error;
}

Error::operator bool() {
	return !error.empty();
}

auto operator<<(std::ostream& os, const Error err) -> std::ostream& {
	return os << err.string();
}

auto Error::check(Error err, const char* file, int line) -> void {
	if(err) {
		std::cerr 
			<< "\033[31;40mUnrecoverable error: \033[37;40m (" 
			<< findLastSlash(file) << ", line " << line << ")\n"
			<< err << '\n';
		std::exit(EXIT_FAILURE);
	}
}
