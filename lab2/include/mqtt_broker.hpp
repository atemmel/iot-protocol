#pragma once
#include "mqtt.hpp"
#include "unix_tcp_socket.hpp"

class MqttBroker {
public:
	auto serve() -> void;
private:
	auto handleClient(UnixTcpSocket client) -> void;
	auto handleSubscription(UnixTcpSocket client, const Mqtt::Message& message) -> void;

	struct Subscription {
		std::string topic;
		Mqtt::QosLevel level;
	};

	struct ClientInfo {
		UnixTcpSocket socket;
		std::vector<Subscription> subscriptions;
	};

	//std::unordered_map<std::string, UnixTcpSocket> connections;
	//std::unordered_map<std::string, std::set<std::string>> subscriptions;
	
	std::vector<ClientInfo> clients;
};
