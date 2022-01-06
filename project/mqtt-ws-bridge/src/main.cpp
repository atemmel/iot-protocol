#include "websocket.hpp"

#include <iostream>
#include <thread>

auto main() -> int {
	ws::Server server;
	server.listen(8001);

	int cpu = 50;
	int mem = 50;
	srand(time(NULL));
	while(true) {
		std::string message;
		message += R"({"cpu":)";
		message += std::to_string(cpu);
		message += R"(, "mem":)";
		message += std::to_string(mem);
		message += "}";
		std::cerr << "Sending message of length: " << message.size() << '\n';
		server.sendToAll(message);

		if(rand() % 2) {
			cpu++;
		} else cpu--;

		if(cpu < 0) cpu = 0;
		if(cpu > 100) cpu = 100;

		if(rand() % 2) {
			mem++;
		} else mem--;

		if(mem < 0) mem = 0;
		if(mem > 100) mem = 100;

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(1s);
	}
}
