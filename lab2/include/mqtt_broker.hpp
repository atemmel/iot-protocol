#pragma once
#include "unix_tcp_socket.hpp"
#include <unordered_map>

class MqttBroker {
public:
	auto serve() -> void;
private:
	auto handleClient(UnixTcpSocket socket) -> void;
	std::unordered_map<std::string, UnixTcpSocket> connections;
};
