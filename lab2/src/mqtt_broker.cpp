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
			std::cerr << err << '\n';
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

			clients.emplace_back(client);
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
			std::cerr << "Message decoding failed: " << error << '\n';
			continue;
		}

		std::cout << message << '\n';

		if(message.type == Mqtt::Type::Subscribe) {
			handleSubscription(client, message);
		} else if(message.type == Mqtt::Publish) {
			//TODO:
		}
	}
}

auto MqttBroker::handleSubscription(UnixTcpSocket client, const Mqtt::Message& message) -> void {
	auto sub = std::get_if<Mqtt::SubscribeHeader>(&message.content);
	uint8_t payload = 0x00;
	auto info = std::find_if(clients.begin(), clients.end(), [&](const ClientInfo& info) {
		return info.socket == client;
	});

	if(sub == nullptr) {
		return;
	}

	if(info == clients.end()) {
		payload = 0x80;
	} else {
		info->subscriptions.reserve(sub->topics.size());
		for(size_t i = 0; i < sub->topics.size(); i++) {
			info->subscriptions.emplace_back(
					sub->topics[i],
					sub->levels[i]
				);
		}
	}

	Mqtt::Message response = {
		.type = Mqtt::Suback,
		.level = Mqtt::Lv0,
		.duplicate = false,
		.retain = false,
		.content = Mqtt::SubackHeader {
			.id = sub->id,
			.payload = payload,
		},
	};

	auto bytes = Mqtt::encode(response);
	client.write(bytes);
}
