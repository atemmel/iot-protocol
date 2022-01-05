#include <iostream>
#include <iomanip>
#include <map>

#include "boilerplate.hpp"
#include "coap.hpp"
#include "unix_udp_socket.hpp"

auto get(const std::string& uri) -> std::string {
	static uint32_t id = 0;
	auto [socket, err] = UnixUdpSocket::create();
	validate(err);
	err = socket.connect("127.0.0.1", 5683);
	validate(err);

	auto message = Coap::Message {
		.type = Coap::Type::Confirmable,
		.code = Coap::Code::Get,
		.id = id++,
		.tokens = {},
		.options = {
			{
				.string = uri,
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

	auto view = std::string(response.payload.begin(),
			response.payload.end());
	return view;
}

auto getCpu() -> std::string {
	return get("cpu");
}

auto getMem() -> std::string {
	return get("mem");
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << argv[0] << " host port" << std::endl;
        return -1;
    }

	MqttClient::setAddressAndPort(argv[1], argv[2]);

	MqttClient::publishHandler = [](const std::string& topic) {
		std::cout << "Publish to topic: " << topic << '\n';
		MqttClient::PublishResponse response;
		response.send = false;
		if(topic == "req/cpu") {
			std::cerr << "Fredde moment\n";
			response.send = true;
			response.topic = "res/cpu";
			response.response = getCpu();
		} else if(topic == "req/mem") {
			response.send = true;
			response.topic = "res/mem";
			response.response = getMem();
		}
		std::cerr << "Sending response...\n";
		return response;
	};

	MqttClient::serve();
}
