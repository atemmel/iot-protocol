#include <fstream>
#include <iomanip>
#include <iostream>
#include "coap.hpp"
#include "unix_tcp_socket.hpp"
#include "unix_udp_socket.hpp"

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
	err = socket.connect("127.0.0.1", 5683);
	validate(err);

	Coap::Header header;
	header.setVersion(1);
	header.setType(0);
	header.setCode(1);
	header.setTokenLength(0);	// AMOUNT of tokens, not amount of bytes that belong to tokens
	header.setMessageId(1234);

	//std::string_view uri = "test";
	std::string_view uri = "basic";
	Coap::Token token;
	token.setType(11); // URI
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
		std::cout << b << ' ';
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

	Coap::Header readHeader;
	auto [bytes, readError] = socket.read(1024);
	validate(writeError);
	std::cout << bytes.size() << " bytes read\n";
	std::cout << std::hex;
	for(int c : bytes) {
		std::cout << c << ' ';
	}
	std::cout << '\n';

	auto headerBytes = BytesView(bytes.begin(), bytes.begin() + sizeof(Coap::Header));
	auto [responseHeader, headerError] = fromBytes<Coap::Header>(headerBytes);
	validate(headerError);

	auto tokenBytes = BytesView(bytes.begin() + sizeof(Coap::Header),
			bytes.begin() + sizeof(Coap::Header) + sizeof(Coap::Token));
	auto [responseToken, tokenError] = fromBytes<Coap::Token>(tokenBytes);
	validate(tokenError);

	std::cout << std::dec << '\n'
		<< "Version: " << responseHeader.getVersion() << '\n'
		<< "Code: " << responseHeader.getCode() << '\n'
		<< "Type: " << responseHeader.getType()  << '\n'
		<< "Length: " << responseHeader.getTokenLength()  << '\n'
		<< "Message id: " << responseHeader.getMessageId() << '\n'
		<< '\n';

	std::cout 
		<< "Type: " << static_cast<int>(responseToken.getType()) << '\n'
		<< "Length: " << static_cast<int>(responseToken.getLength()) << '\n';

	if(responseToken.getType() == 15) {
		auto remainingBytes = BytesView(bytes.begin() + sizeof(Coap::Header) + sizeof(Coap::Token),
				bytes.begin() + sizeof(Coap::Header) + sizeof(Coap::Token) + responseToken.getLength());

		std::string view(remainingBytes.begin(), remainingBytes.end());
		std::cout << view << '\n';
	}
}
