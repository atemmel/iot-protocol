#include <fstream>
#include <iomanip>
#include <iostream>
#include "coap.hpp"
#include "unix_tcp_socket.hpp"
#include "unix_udp_socket.hpp"

auto main() -> int {
	auto [socket, err] = UnixUdpSocket::create();
	validate(err);
	//err = socket.connect("coap.me", 5683);
	err = socket.connect("127.0.0.1", 5683);
	validate(err);

	Coap::Message message = {
		.type = Coap::Type::Confirmable,
		.code = Coap::Code::Get,
		.id = 1234,
		.tokens = {},
		.options = {
			{
				.string = "basic",
				.type = Coap::OptionType::UriPath,
			}
		},
		.payload = {},
	};

	auto bytes = Coap::encode(message);
	uint32_t len;
	std::tie(len, err) = socket.write(bytes);
	validate(err);

	std::tie(bytes, err) = socket.read(1024);
	validate(err);

	auto [response, decodeError] = Coap::decode(bytes);
	validate(decodeError);

	std::cout 
		<< "Type: " << response.type << '\n'
		<< "Code: " << response.code << '\n'
		<< "Id: " << response.id << "\nOptions:\n";

	for(const auto& opt : response.options) {
		std::cout << '\t' << opt.type << ' ';
		if(opt.isInteger()) {
			std::cout << opt.integer;
		} else if(opt.isString()) {
			std::cout << opt.string;
		}
		std::cout << '\n';
	}

	std::cout << "Payload size: " << response.payload.size() << '\n';
}
