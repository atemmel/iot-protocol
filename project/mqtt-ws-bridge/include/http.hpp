#pragma once

#include "tcp_socket.hpp"

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

struct Http {
	enum Method : int32_t {
		Get,
		Post,
		Delete,
		Put,
	};

	struct Message {
		std::string path;
		std::unordered_map<std::string, std::string> headers;
		Method method;

		auto setMethod(std::string_view view) -> void;
	};

	static auto parseMessage(TcpSocket client) -> Result<Message>;

	class Server {
	public:
		using Handler = std::function<void(const Message& request, TcpSocket client)>;
		auto listen() -> Result<void>;
		auto endpoint(std::string_view path, Handler callback) -> void;
		auto fallbackEndpoint(Handler callback) -> void;
	private:
		auto handleClient(TcpSocket client) -> void;
		std::unordered_map<std::string, Handler> handlers;
		Handler fallbackHandler;
	};
};
