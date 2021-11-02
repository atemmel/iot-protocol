#include <iostream>
#include "unix_tcp_socket.hpp"
#include "unix_udp_socket.hpp"

template<typename T>
struct IntoBytes {
	IntoBytes(const T& underlying) : underlying(underlying) {}
	
	const Byte* begin() const {
		return reinterpret_cast<const Byte*>(this);
	}

	const Byte* end() const {
		return reinterpret_cast<const Byte*>(this + sizeof(underlying));
	}

private:
	const T& underlying;
};

struct CoapHeader {
	unsigned int ver : 2,
		 type : 2,
		 tokenLength : 4,
		 code : 8,
		 msgId : 16;
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

	/*
	CoapHeader header;
	header.ver = 0;
	header.tokenLength = 0;
	header.type = 0;
	header.msgId = 0;
	header.code = 0;

	std::cout << "Header size: " << sizeof header << '\n';
	std::cout << "Writing data...\n";
	auto [len, writeError] = socket.write(IntoBytes(header));
	validate(writeError);
	std::cout << len << " bytes written\n";

	auto [bytes, readError] = socket.read(1024);
	validate(writeError);
	std::cout << bytes.data() << '\n';
}
