#include "mqtt_broker.hpp"

#include <thread>

#include "mqtt.hpp"
#include "unix_tcp_socket.hpp"

auto MqttBroker::serve() -> void {
	auto [listener, err ] = UnixTcpSocket::create();
	validate(err);

	//https://mqtt.org/faq/
	err = listener.listen(1883);
	validate(err);

	while(true) {
		auto [client, err] = listener.accept();
		if(err) {
			std::cerr << err.string() << '\n';
		} else {
			std::thread thread(&MqttBroker::handleClient, this, client);
			thread.detach();
		}
	}
}

auto MqttBroker::handleClient(UnixTcpSocket client) -> void {
	auto [message, error] = Mqtt::decode(client);
	if(error) {
		std::cerr << error << '\n';
		return;
	}

	if(auto connect = std::get_if<Mqtt::ConnectHeader>(&message.content); connect) {
		
		Mqtt::Message response = {
			.type = Mqtt::Connack,
			.level = Mqtt::Lv0,
			.duplicate = false,
			.retain = false,
		};

		if(connect->protocol != "MQTT") {
			// unknown protocol
			response.content = Mqtt::ConnackHeader{
				.code = 0x02,
			};
		} else if(connect->version != 4) {
			// unknown protocol version
			response.content = Mqtt::ConnackHeader{
				.code = 0x01,
			};
		} else {
			// success
			std::cout << "New client: " << message << '\n';
			response.content = Mqtt::ConnackHeader{
				.code = 0x00,
			};
			connections.insert({connect->identifier, client});
		}

		auto bytes = Mqtt::encode(response);
		client.write(bytes);
	} else {
		std::cerr << "Malformed client connection attempt\n";
		return;
	};

	while(true) {
		std::tie(message, error) = Mqtt::decode(client);
		if(error) {
			std::cerr << error << '\n';
		}

		std::cout << message << '\n';
	}
}
