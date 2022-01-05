#pragma once

#include "result.hpp"
#include "tcp_socket.hpp"
#include "utils.hpp"

#include <cstdint>
#include <set>
#include <thread>
#include <mutex>
#include <vector>

struct ws {

	struct Frame {
		enum Opcode : uint8_t {
			Continuation = 0x0,
			Text = 0x1,
			Binary = 0x2,
			Close = 0x8,
			Ping = 0x9,
			Pong = 0xA,
		};

		std::vector<Byte> payload;
		Opcode op;
		bool fin;
	};

	static auto decode(TcpSocket client) -> Result<Frame>;
	static auto encode(const Frame& frame) -> std::vector<Byte>;

	class Server {
	public:
		auto listen(uint16_t port) -> Result<void>;
		auto sendToAll(std::string_view message) -> void;
	private:
		auto listenThread() -> void;
		auto handleClient(TcpSocket client) -> void;
		TcpSocket socket;

		std::set<TcpSocket> clients;
		std::mutex clientsMutex;
	};

};
