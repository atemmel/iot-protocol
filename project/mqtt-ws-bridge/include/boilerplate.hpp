#pragma once

#include <mqtt_client_cpp.hpp>
#include <mqtt/tcp_endpoint.hpp>

#include <functional>
#include <memory>
#include <string>

namespace MqttClient {

using Wtf = std::shared_ptr<mqtt::callable_overlay<mqtt::sync_client<mqtt::tcp_endpoint<as::ip::tcp::socket, as::io_context::strand>>>>;
extern Wtf c;

struct PublishResponse {
	std::string topic;
	std::string response;
	bool send;
};

auto setAddressAndPort(const std::string& address, const std::string& port) -> void;
auto serve() -> void;
extern std::function<PublishResponse(const std::string&, const std::string&)> publishHandler;
extern std::string address;
extern std::string port;

};
