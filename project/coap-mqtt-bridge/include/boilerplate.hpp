#pragma once

#include <mqtt_client_cpp.hpp>
#include <mqtt/tcp_endpoint.hpp>

#include <functional>
#include <memory>
#include <string>

namespace MqttClient {

struct PublishResponse {
	std::string topic;
	std::string response;
	bool send;
};


auto setAddressAndPort(const std::string& address, const std::string& port) -> void;
auto serve() -> void;
extern std::function<PublishResponse(const std::string&)> publishHandler;
extern std::string address;
extern std::string port;

};
