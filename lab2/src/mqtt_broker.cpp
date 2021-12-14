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

		switch(message.type) {
			case Mqtt::Type::Subscribe:
				handleSubscription(client, message);
				break;
			case Mqtt::Publish:
				handlePublish(message);
				break;
			case Mqtt::Unsubscribe:
				handleUnsubscribe(client, message);
				break;
			case Mqtt::Pingreq:
				handlePingreq(client);
				break;
			case Mqtt::Disconnect:
				handleDisconnect(client);
				return;
			default:
				std::cerr << "Unsupported...\n";
				break;
		}
	}
}

auto MqttBroker::handleSubscription(UnixTcpSocket client, const Mqtt::Message& message) -> void {
	auto sub = std::get_if<Mqtt::SubscribeHeader>(&message.content);
	Mqtt::SubackHeader suback = {
		.id = sub->id,
	};
	suback.payload.resize(sub->levels.size(), 0x00);

	clientsMutex.lock();

	if(sub == nullptr) {
		clientsMutex.unlock();
		return;
	}

	for(size_t i = 0; i < sub->topics.size(); i++) {
		subscriptions[sub->topics[i]].insert({
			.socket = client,
			.level = sub->levels[i],
		});
	}

	clientsMutex.unlock();

	Mqtt::Message response = {
		.type = Mqtt::Suback,
		.level = Mqtt::Lv0,
		.duplicate = false,
		.retain = false,
		.content = suback,
	};

	auto bytes = Mqtt::encode(response);
	auto [_, err] = client.write(bytes);
	if(err) {
		std::cerr << err << '\n';
	}
}

auto MqttBroker::handlePublish(const Mqtt::Message& message) -> void {
	auto publish = std::get_if<Mqtt::PublishHeader>(&message.content);
	if(publish == nullptr) {
		return;
	}

	auto bytes = Mqtt::encode(message);
	auto doPublish = [&](const std::set<Subscription>& set) {
		for(const auto& sub : set) {
			auto [_, err] = sub.socket.write(bytes);
			if(err) {
				std::cerr << err << '\n';
			}
		}
	};

	clientsMutex.lock();

	auto& subscribers = subscriptions[publish->topic];
	doPublish(subscribers);
	subscribers = subscriptions["#"];
	doPublish(subscribers);

	clientsMutex.unlock();
}

auto MqttBroker::handleUnsubscribe(UnixTcpSocket client, const Mqtt::Message& message) -> void {
	auto unsub = std::get_if<Mqtt::UnsubscribeHeader>(&message.content);
	if(unsub == nullptr) {
		return;
	}

	clientsMutex.lock();

	for(const auto& topic : unsub->topics) {
		auto it = subscriptions.find(topic);
		if(it != subscriptions.end()) {
			if(it->second.contains({client})) {
				it->second.erase({client});
			}
		}
	}

	clientsMutex.unlock();
}

auto MqttBroker::handlePingreq(UnixTcpSocket client) -> void {
	Mqtt::Message response = {
		.type = Mqtt::Pingresp,
		.level = Mqtt::Lv0,
		.duplicate = false,
		.retain = false,
		.content = {},
	};

	auto bytes = Mqtt::encode(response);
	auto [_, err] = client.write(bytes);
	if(err) {
		std::cerr << "Ping error: " << err << '\n';
	}
}

auto MqttBroker::Subscription::operator<(const Subscription& other) const -> bool {
	return socket < other.socket;
}

auto MqttBroker::Subscription::operator==(const Subscription& other) const -> bool {
	return socket == other.socket;
}

auto MqttBroker::handleDisconnect(UnixTcpSocket client) -> void {
	unsubscribeClient(client);
	client.close();
}

auto MqttBroker::unsubscribeClient(UnixTcpSocket client) -> void {
	clientsMutex.lock();
	for(auto& pair : subscriptions) {
		if(pair.second.contains({client})) {
			pair.second.erase({client});
		}
	}
	clientsMutex.unlock();
}
