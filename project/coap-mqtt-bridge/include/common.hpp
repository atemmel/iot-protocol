#pragma once
#include <algorithm>
#include <cstdlib>
#include <vector>

#include <iostream>

#include "error.hpp"

using Byte = unsigned char;
using Bytes = std::vector<Byte>;

template<typename T>
class View {
public:
	template<typename U>
	View(const U* a, size_t length) : first(a), last(a + length) {}

	template<typename U>
	View(const U a, const U b) : first(&*a), last(&*b) {}

	template<typename Container>
	View(const Container& container) : 
		first(reinterpret_cast<const T*>(&*container.begin())), 
		last(reinterpret_cast<const T*>(&*container.end())) {}

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

template<typename T, std::enable_if_t<std::is_integral<T>::value, bool> = true>
auto getIntSegment(T data, T mask, T shift) {
	return (data & mask) >> shift;
}

template<typename T, std::enable_if_t<std::is_integral<T>::value, bool> = true>
auto setIntSegment(T& data, T value, T mask, T shift) {
	data = (data & ~mask) | ((value << shift) & mask);
}

template<typename T>
struct AsBytes {
	AsBytes(const T& underlying) : underlying(underlying) {}

	struct Iterator;
	
	auto begin() const -> Iterator {
		auto base = reinterpret_cast<const Byte*>(&underlying);
		return Iterator(base + sizeof underlying - 1);
	}

	auto end() const -> Iterator {
		auto base = reinterpret_cast<const Byte*>(&underlying);
		return Iterator(base - 1);
	}

	auto size() const -> size_t {
		return begin() - end();
	}

	struct Iterator {
		using iterator_category = std::input_iterator_tag;
		using value_type = Byte;
		using difference_type = std::ptrdiff_t;
		using pointer = Byte*;
		using reference = Byte&;

		Iterator() = default;
		Iterator(const Byte* address) : ptr(address) {
		}

		auto operator==(Iterator rhs) const -> bool{
			return ptr == rhs.ptr;
		}

		auto operator!=(Iterator rhs) const -> bool {
			return ptr != rhs.ptr;
		}

		auto operator*() const -> Byte {
			return *ptr;
		}

		auto operator++() -> Iterator{
			return ptr--;
		}

		auto operator--() -> Iterator{
			return ptr++;
		}

		auto operator-(Iterator rhs) const -> std::ptrdiff_t {
			return ptr - rhs.ptr;
		}

	private:
		const Byte* ptr = nullptr;
	};

private:
	const T& underlying;
};

template<typename T>
auto fromBytes(BytesView bytes) -> std::tuple<T, Error> {
	T value;
	Byte* valueBegin = reinterpret_cast<Byte*>(&value);

	if(sizeof(T) != bytes.size()) {
		return {
			T(),
			"Size mismatch between byte view and conversion type",
		};
	} else if(bytes.data() == nullptr) {
		return {
			T(),
			"Byte view points to null",
		};
	}

	auto byteBegin = bytes.end() - 1;
	auto byteEnd = bytes.begin() -1;
	while(byteBegin != byteEnd) {
		*valueBegin++ = *byteBegin--;
	}

	return {
		value,
		nullptr,
	};
};
