#pragma once
#include <algorithm>
#include <cstdlib>
#include <vector>

using Byte = unsigned char;
using Bytes = std::vector<Byte>;

template<typename T>
class View {
public:
	template<typename U>
	View(const U* a, const U* b) : first(a), last(b) {}

	template<typename U>
	View(const U* a, size_t length) : first(a), last(a + length) {}

	template<typename Container>
	View(const Container& container) : 
		first(reinterpret_cast<const unsigned char*>(&*container.begin())), 
		last(reinterpret_cast<const unsigned char*>(&*container.end())) {}

	auto size() const -> size_t {
		return std::distance(first, last);
	}

	auto begin() const -> const T* {
		return first;
	}

	auto end() const -> const T* {
		return last;
	}

	auto data() const -> const T* {
		return first;
	}

	auto operator[](size_t index) const -> const T& {
		return first[index];
	}

private:
	const T* first;
	const T* last;
};

using BytesView = View<Byte>;
