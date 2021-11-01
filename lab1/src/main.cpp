#include <iostream>
#include "unix_tcp_socket.hpp"

auto main() -> int {
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

	/*
	auto [bytes, readError] = socket.readUntil('\n');
	validate(readError);
	std::cout << bytes.data() << '\n';
	*/
}
