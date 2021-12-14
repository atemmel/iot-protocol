#pragma once
#include "mqtt.hpp"
#include "unix_tcp_socket.hpp"

#include <set>
#include <thread>
#include <unordered_map>

class MqttBroker {
public:
	auto serve() -> void;
private:
	auto handleClient(UnixTcpSocket client) -> void;
	auto handleSubscription(UnixTcpSocket client, const Mqtt::Message& message) -> void;
	auto handlePublish(const Mqtt::Message& message) -> void;
	auto handleUnsubscribe(UnixTcpSocket client, const Mqtt::Message& message) -> void;
	auto handlePingreq(UnixTcpSocket client) -> void;
	auto handleDisconnect(UnixTcpSocket client) -> void;

	auto unsubscribeClient(UnixTcpSocket client) -> void;

	struct Subscription {
		UnixTcpSocket socket;
		Mqtt::QosLevel level;

		auto operator<(const Subscription& other) const -> bool;
		auto operator==(const Subscription& other) const -> bool;
	};

	/*
	struct ClientInfo {
		UnixTcpSocket socket;
		std::vector<Subscription> subscriptions;
	};
	*/
	
	//std::vector<ClientInfo> clients;
	std::unordered_map<std::string, std::set<Subscription>> subscriptions;
	std::mutex clientsMutex;
};
