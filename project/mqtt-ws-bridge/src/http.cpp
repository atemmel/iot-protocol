#include "http.hpp"

#include <iostream>

auto Http::Message::setMethod(std::string_view view) -> void {
	if(view == "POST") {
		method = Post;
	} else if(view == "GET") {
		method = Get;
	} else if(view == "DELETE") {
		method = Delete;
	} else if(view == "PUT") {
		method = Put;
	}
}

auto Http::parseMessage(TcpSocket client) -> Result<Message> {
	Result<Message> result;
	auto protocol = client.readUntil('\n');

	auto pathBegin = protocol.value.find(' ');
	auto pathEnd = protocol.value.find_last_of(' ');
	if(pathBegin == -1) {
		return result.set("No protocol found in header");
	}

	auto method = std::string_view(protocol.value.begin(),
			protocol.value.begin() + pathBegin);

	auto path = std::string_view(protocol.value.begin() + pathBegin + 1,
			protocol.value.begin() + pathEnd);

	result.value.setMethod(method);
	result.value.path = path;

	while(true) {
		auto header = client.readUntil('\n');
		auto split = header.value.find(':');
		if(split == -1 || header.value.empty()) {
			break;
		}

		if(header.value.back() == '\r') {
			header.value.pop_back();
		}

		auto key = std::string_view(header.value.begin(), header.value.begin() + split);
		split++;
		while(!std::isgraph(header.value[split])) {
			split++;
		}
		auto value = std::string_view(header.value.begin() + split, header.value.end());

		result.value.headers.emplace(key, value);
	}

	return result;
}

auto Http::Server::listen() -> Result<void> {
	Result<void> toReturn;
	auto result = TcpSocket::create();
	
	if(result.fail()) {
		return toReturn.set(result.reason());
	}

	auto socket = result.value;
	auto voidRes = socket.listen(3500);
	if(voidRes.fail()) {
		return toReturn.set(voidRes.reason());
	}

	while(true) {
		auto client = socket.accept();
		if(client.fail()) {
			std::cerr << client.reason() << '\n';
		} else {
			handleClient(client.value);
		}
	}
}
auto Http::Server::endpoint(std::string_view path, Handler callback) -> void {
	handlers.emplace(path, callback);
}

auto Http::Server::fallbackEndpoint(Handler callback) -> void {
	fallbackHandler = callback;
}

auto Http::Server::handleClient(TcpSocket client) -> void {
	auto result = parseMessage(client);
	if(result.fail()) {
		std::cerr << result.reason() << '\n';
		client.close();
		return;
	}


	auto message = std::move(result.value);

	auto handler = handlers.find(message.path);
	if(handler != handlers.end()) {
		handler->second(message, client);
	} else {
		fallbackHandler(message, client);
	}
	client.close();
};
