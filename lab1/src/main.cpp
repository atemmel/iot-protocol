#include <fstream>
#include <iomanip>
#include <iostream>
#include "coap.hpp"
#include "unix_tcp_socket.hpp"
#include "unix_udp_socket.hpp"

auto sendMessage(UnixUdpSocket& socket, const Coap::Message& message) {
	Error err = nullptr;

	std::cout << "Sending: " << message << '\n';

	auto bytes = Coap::encode(message);
	uint32_t len;

	std::tie(len, err) = socket.write(bytes);
	validate(err);

	std::tie(bytes, err) = socket.read(1024);
	validate(err);

	auto [response, decodeError] = Coap::decode(bytes);
	validate(decodeError);

	std::cout << "Response: " << response << '\n';
	auto view = std::string(response.payload.begin(),
		response.payload.end());
	std::cout << view << "\n\n";
}

auto main() -> int {
	auto [socket, err] = UnixUdpSocket::create();
	validate(err);
	err = socket.connect("coap.me", 5683);
	validate(err);

	const Coap::Message getRequest = {
		.type = Coap::Type::Confirmable,
		.code = Coap::Code::Get,
		.id = 1234,
		.tokens = {},
		.options = {
			{
				.string = "test",
				.type = Coap::OptionType::UriPath,
			}
		},
		.payload = {},
	};

	const Coap::Message deleteRequest = {
		.type = Coap::Type::Confirmable,
		.code = Coap::Code::Delete,
		.id = 1234,
		.tokens = {},
		.options = {
			{
				.string = "test",
				.type = Coap::OptionType::UriPath,
			}
		},
		.payload = {},
	};

	const Coap::Message postRequest = {
		.type = Coap::Type::Confirmable,
		.code = Coap::Code::Post,
		.id = 1234,
		.tokens = {},
		.options = {
			{
				.string = "test",
				.type = Coap::OptionType::UriPath,
			}
		},
		.payload = {},
	};

	const std::string_view payloadString = "woop woop";
	Bytes payload(payloadString.size());
	std::copy(payloadString.begin(), payloadString.end(),
			payload.begin());

	const Coap::Message putRequest = {
		.type = Coap::Type::Confirmable,
		.code = Coap::Code::Put,
		.id = 1234,
		.tokens = {},
		.options = {
			{
				.string = "test",
				.type = Coap::OptionType::UriPath,
			}
		},
		.payload = payload,
	};

	// OK
	std::cout << "Get request:\n";
	sendMessage(socket, getRequest);

	// OK
	std::cout << "Delete request:\n";
	sendMessage(socket, deleteRequest);

	std::cout << "Post request:\n";
	sendMessage(socket, postRequest);

	std::cout << "Put in sink:\n";
	sendMessage(socket, putRequest);

	Coap::Message getSinkMessage = {
		.type = Coap::Type::Confirmable,
		.code = Coap::Code::Get,
		.id = 1235,
		.tokens = {},
		.options = {
			{
				.string = "sink",
				.type = Coap::OptionType::UriPath,
			}
		},
		.payload = {},
	};

	std::cout << "Get from sink:\n";
	sendMessage(socket, getSinkMessage);

	std::cout << "Delete from sink:\n";
	sendMessage(socket, Coap::Message{
		.type = Coap::Type::Confirmable,
		.code = Coap::Code::Delete,
		.id = 1236,
		.tokens = {},
		.options = {
			{
				.string = "sink",
				.type = Coap::OptionType::UriPath,
			}
		},
	});

	std::cout << "Get from sink:\n";
	sendMessage(socket, getSinkMessage);
}
