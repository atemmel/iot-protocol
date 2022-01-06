#include "websocket.hpp"

#include "boilerplate.hpp"

#include <iostream>
#include <thread>

auto main(int argc, char** argv) -> int {
	if (argc != 3) {
        std::cout << argv[0] << " host port" << std::endl;
        return -1;
    }

	MqttClient::setAddressAndPort(argv[1], argv[2]);
	ws::Server server;
	server.listen(8001);

	float cpu = 0.f;
	float mem = 0.f;
	bool justGotCpu = false;
	bool justGotMem = false;
	auto askedForCpu = std::chrono::high_resolution_clock::now();
	auto askedForMem = std::chrono::high_resolution_clock::now();
	auto recievedCpu = std::chrono::high_resolution_clock::now();
	auto recievedMem = std::chrono::high_resolution_clock::now();

	auto sendWsMessage = [&](float cpu, float mem, float rtt) {
		std::string message;
		message += R"({"cpu":)";
		message += std::to_string(cpu);
		message += R"(, "mem":)";
		message += std::to_string(mem);
		message += R"(, "rtt":)";
		message += std::to_string(rtt);
		message += "}";
		std::cerr << "Sending message of length: " << message.size() << '\n';
		server.sendToAll(message);
	};

	MqttClient::publishHandler = [&](const std::string& topic, const std::string& content) {
		std::cout << "Publish to topic: " << topic << '\n';
		MqttClient::PublishResponse response;
		response.send = false;
		if(topic == "res/cpu") {
			recievedCpu = std::chrono::high_resolution_clock::now();
			cpu = std::stof(content);
			justGotCpu = true;

		} else if(topic == "res/mem") {
			recievedMem = std::chrono::high_resolution_clock::now();
			mem = std::stof(content);
			justGotMem = true;
		}

		std::cerr << "Sending response...\n";
		return response;
	};

	auto mqttThread = std::thread(MqttClient::serve);
	mqttThread.detach();

	while(true) {
		using namespace std::chrono_literals;
		float rtt = 0.f;

		auto askForCpu = std::thread([&]() {
			justGotCpu = false;
			askedForCpu = std::chrono::high_resolution_clock::now();
			std::cerr << "Sending publish to req/cpu\n";
			MqttClient::c->publish("req/cpu", " ");
			while(!justGotCpu) {
				std::this_thread::sleep_for(1s);
			}
			
			auto diff = recievedCpu - askedForCpu;
			auto diffAsMs = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
			std::cerr << "(CPU) Time Difference: " << diffAsMs << '\n';
			rtt += diffAsMs;
		});

		askForCpu.join();

		auto askForMem = std::thread([&]() {
			justGotMem = false;
			askedForMem = std::chrono::high_resolution_clock::now();
			std::cerr << "Sending publish to req/mem\n";
			MqttClient::c->publish("req/mem", " "); 
			while(!justGotMem) {
				std::this_thread::sleep_for(1s);
			}
			
			auto diff = recievedMem - askedForMem;
			auto diffAsMs = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
			std::cerr << "(MEM) Time Difference: " << diffAsMs << '\n';
			rtt += diffAsMs;
		});

		askForMem.join();

		sendWsMessage(cpu, mem, rtt / 2.f);

		std::this_thread::sleep_for(1s);
	}
}
