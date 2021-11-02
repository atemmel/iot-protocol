#include <fstream>
#include <iomanip>
#include <iostream>
#include "unix_tcp_socket.hpp"
#include "unix_udp_socket.hpp"

struct CoapHeader {
	uint32_t data = 0;

	void setVersion(uint8_t version) {
		constexpr auto mask = 0b11000000000000000000000000000000;
		constexpr auto shift = 30;
		data = (data & ~mask) | ((version << shift) & mask);
	}

	void setType(uint8_t type) {
		constexpr auto mask = 0b00110000000000000000000000000000;
		constexpr auto shift = 28;
		data = (data & ~mask) | ((type << shift) & mask);
	}

	void setTokenLength(uint8_t length) {
		constexpr auto mask = 0b00001111000000000000000000000000;
		constexpr auto shift = 24;
		data = (data & ~mask) | ((length << shift) & mask);
	}

	void setCode(uint8_t code) {
		constexpr auto mask = 0b00000000111100000000000000000000;
		constexpr auto shift = 16;
		data = (data & ~mask) | ((code << shift) & mask);
	}

	void setMessageId(uint16_t id) {
		constexpr auto mask = 0b00000000000001111111111111111111;
		constexpr auto shift = 0;
		data = (data & ~mask) | ((id << shift) & mask);
	}
};

struct CoapToken {
	uint8_t data = 0;

	void setType(uint8_t type) {
		constexpr auto mask = 0xF0;
		constexpr auto shift = 4;
		data = (data & ~mask) | ((type << shift) & mask);
	}

	void setLength(uint8_t length) {
		constexpr auto mask = 0x0F;
		constexpr auto shift = 0;
		data = (data & ~mask) | ((length << shift) & mask);
	}
};


template<typename T>
struct AsBytes {
	AsBytes(const T& underlying) : underlying(underlying) {}
	
	const Byte* begin() const {
		return reinterpret_cast<const Byte*>(&underlying);
	}

	const Byte* end() const {
		return reinterpret_cast<const Byte*>(&underlying) + sizeof(underlying);
	}

	uint32_t length() const {
		return end() - begin();
	}

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
	err = socket.connect("coap.me", 5683);
	validate(err);

	CoapHeader header;
	header.setVersion(1);
	header.setType(0);
	header.setTokenLength(1);	// AMOUNT of tokens, not amount of bytes that belong to tokens
	header.setCode(1);
	header.setMessageId(1234);

	std::string_view uri = "test";
	CoapToken token;
	token.setType(9); // URI
	token.setLength(uri.length());

	std::cout << "Writing data...\n";
	auto [len, writeError] = socket.write(AsBytes(header));
	validate(writeError);
	std::cout << len << " bytes written\n";
	std::cout << "Token size: " << sizeof token << '\n';
	std::tie(len, writeError) = socket.write(AsBytes(token));
	validate(writeError);
	std::cout << len << " bytes written\n";
	std::tie(len, writeError) = socket.write(uri);
	validate(writeError);
	std::cout << len << " bytes written\n";
	std::cout << "Done writing...\n";

	auto [bytes, readError] = socket.read(1);
	validate(writeError);
	std::cout << bytes.data() << '\n';
}
