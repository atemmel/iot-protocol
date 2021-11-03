#include <fstream>
#include <iomanip>
#include <iostream>
#include "unix_tcp_socket.hpp"
#include "unix_udp_socket.hpp"

struct CoapHeader {
	uint32_t data = 0;

	auto setVersion(uint8_t version) {
		constexpr auto mask = 0b11000000000000000000000000000000;
		constexpr auto shift = 30;
		data = (data & ~mask) | ((version << shift) & mask);
	}

	auto setType(uint8_t type) {
		constexpr auto mask = 0b00110000000000000000000000000000;
		constexpr auto shift = 28;
		data = (data & ~mask) | ((type << shift) & mask);
	}

	auto setTokenLength(uint8_t length) {
		constexpr auto mask = 0b00001111000000000000000000000000;
		constexpr auto shift = 24;
		data = (data & ~mask) | ((length << shift) & mask);
	}

	auto setCode(uint8_t code) {
		constexpr auto mask = 0b00000000111100000000000000000000;
		constexpr auto shift = 16;
		data = (data & ~mask) | ((code << shift) & mask);
	}

	auto setMessageId(uint16_t id) {
		constexpr auto mask = 0b00000000000001111111111111111111;
		constexpr auto shift = 0;
		data = (data & ~mask) | ((id << shift) & mask);
	}
};

struct CoapToken {
	uint8_t data = 0;

	auto setType(uint8_t type) {
		constexpr auto mask = 0xF0;
		constexpr auto shift = 4;
		data = (data & ~mask) | ((type << shift) & mask);
	}

	auto setLength(uint8_t length) {
		constexpr auto mask = 0x0F;
		constexpr auto shift = 0;
		data = (data & ~mask) | ((length << shift) & mask);
	}
};


template<typename T>
struct AsBytes {
	AsBytes(const T& underlying) : underlying(underlying) {}

	struct Iterator;
	
	auto begin() const -> Iterator {
		auto base = reinterpret_cast<const Byte*>(&underlying);
		return Iterator(base + sizeof underlying - 1);
	}

	auto end() const -> Iterator {
		//return reinterpret_cast<const Byte*>(&underlying) + sizeof(underlying);
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

auto main() -> int {
	/*
	auto [socket, err] = UnixTcpSocket::create();
	validate(err);

	err = socket.connect("coap.me", 80);
	//err = socket.connect("coap.me", 5683);
	validate(err);

	std::string httpRequest = "GET / HTTP/1.0\r\n\r\n";

	auto asBytes = BytesView(httpRequest);
	std::cout << "Writing data...\n";
	auto [len, writeError] = socket.write(asBytes);
	validate(writeError);
	std::cout << len << " bytes written\n";

	auto [bytes, readError] = socket.read(1024);
	validate(writeError);
	std::cout << bytes.data() << '\n';
	*/

	auto [socket, err] = UnixUdpSocket::create();
	validate(err);
	//err = socket.connect("coap.me", 5683);
	err = socket.connect("localhost", 5683);
	validate(err);

	CoapHeader header;
	header.setVersion(1);
	header.setType(0);
	header.setTokenLength(1);	// AMOUNT of tokens, not amount of bytes that belong to tokens
	header.setCode(1);
	header.setMessageId(1234);

	std::string_view uri = "/basic";
	CoapToken token;
	token.setType(9); // URI
	token.setLength(uri.length());

	auto headerAsBytes = AsBytes(header);
	auto tokenAsBytes = AsBytes(token);
	auto uriAsBytes = BytesView(uri);

	Bytes message;
	message.reserve(headerAsBytes.size() + tokenAsBytes.size() + uriAsBytes.size());
	message.insert(message.end(), headerAsBytes.begin(), headerAsBytes.end());
	message.insert(message.end(), tokenAsBytes.begin(), tokenAsBytes.end());
	message.insert(message.end(), uriAsBytes.begin(), uriAsBytes.end());
	for(int b : message) {
		std::cout << std::hex << std::setw(2) << std::setfill('0') << b << ' ';
	}
	std::cout << '\n';
	std::cout << "Header: " << header.data << '\n';
	std::cout << "Token: " << static_cast<int>(token.data) << '\n';
	std::cout << "Uri: " << uri << " 0x";
	for (auto c : uriAsBytes) {
		std::cout << static_cast<int>(c) << ' ';
	}
	std::cout << '\n';
	auto [len, writeError] = socket.write(message);
	validate(writeError);
	std::cout << std::dec << len << " bytes written\n";
	std::cout << "Done writing...\n";

	auto [bytes, readError] = socket.read(16);
	validate(writeError);
	std::cout << std::hex;
	for(int c : bytes) {
		std::cout << c << ' ';
	}
	std::cout << '\n';
}
