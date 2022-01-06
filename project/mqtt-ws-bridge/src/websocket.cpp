#include "websocket.hpp"

#include "base64.hpp"
#include "http.hpp"
#include "sha1.hpp"

#include <bitset>
#include <iostream>

auto ws::decode(TcpSocket client) -> Result<Frame> {
	Result<Frame> result;
	
	auto readHeader = client.readBytes(2);
	if(readHeader.fail()) {
		return result.set(readHeader.reason());
	}

	auto header = std::move(readHeader.value);

	auto b0 = header[0];
	auto b1 = header[1];

	result.value.fin = (b0 & 0x1);
	result.value.op = static_cast<Frame::Opcode>(b0 & 0x0F);

	bool hasMask = b1 & 0x80;

	uint64_t payloadLength = (b1 & 0x7F);
	if(payloadLength == 125) {
		goto FULL_LENGTH_AQUIRED;
	}

	if(payloadLength == 126) {
		auto readExtendedLength = client.readBytes(2);
		if(readExtendedLength.fail()) {
			return result.set(readExtendedLength.reason());
		}

		uint32_t extendedLength = readExtendedLength.value[1];
		extendedLength += readExtendedLength.value[0] << 8;
		payloadLength += extendedLength;

		goto FULL_LENGTH_AQUIRED;
	}

	if(payloadLength == 127) {
		auto readExtendedLength = client.readBytes(8);
		if(readExtendedLength.fail()) {
			return result.set(readExtendedLength.reason());
		}

		uint64_t extendedLength = 0;
		for(int i = 0; i < 8; i++) {
			extendedLength |= readExtendedLength.value[i] << (i * 8);
		}
		payloadLength += extendedLength;

		goto FULL_LENGTH_AQUIRED;
	}

FULL_LENGTH_AQUIRED:

	std::vector<Byte> mask;
	if(hasMask) {
		auto readMaskingKey = client.readBytes(4);
		if(readMaskingKey.fail()) {
			return result.set(readMaskingKey.reason());
		}

		mask = std::move(readMaskingKey.value);
	}

	auto readRemainder = client.readBytes(payloadLength);
	if(readRemainder.fail()) {
		return result.set(readRemainder.reason());
	}

	auto remainder = std::move(readRemainder.value);
	auto& decoded = result.value.payload;
	decoded.resize(remainder.size());

	if(hasMask) {
		for(uint64_t i = 0; i < remainder.size(); i++) {
			decoded[i] = static_cast<char>(remainder[i] ^ mask[i & 3]);
		}
	} else {
		std::copy(remainder.begin(), remainder.end(), decoded.begin());
	}

	return result;
}

auto ws::encode(const Frame& frame) -> std::vector<Byte> {
	std::vector<Byte> bytes;

	Byte b0 = frame.op;
	b0 |= 0x80;
	
	auto payloadLength = frame.payload.size();
	Byte b1 = (payloadLength);
	b1 &= 0x7F;

	bytes.push_back(b0);
	bytes.push_back(b1);
	bytes.reserve(bytes.size() + payloadLength);
	bytes.insert(bytes.end(), frame.payload.begin(), frame.payload.end());
	return bytes;
}

auto ws::Server::listen(uint16_t port) -> Result<void> {
	Result<void> toReturn;
	auto result = TcpSocket::create();
	
	if(result.fail()) {
		return toReturn.set(result.reason());;
	}

	socket = result.value;
	toReturn = socket.listen(port);
	if(toReturn.fail()) {
		return toReturn;
	}

	std::thread doListen(&ws::Server::listenThread, this);
	doListen.detach();
	return toReturn;
}

auto ws::Server::sendToAll(std::string_view message) -> void {
	Frame frame = {
		.payload = {message.begin(), message.end()},
		.op = Frame::Text,
		.fin = true,
	};

	auto bytes = encode(frame);

	clientsMutex.lock();
	for(auto client : clients) {
		client.write(bytes);
	}
	clientsMutex.unlock();
}

auto ws::Server::listenThread() -> void {
	while(true) {
		auto client = socket.accept();
		if(client.fail()) {
			std::cerr << client.reason() << '\n';
		} else {
			auto thread = std::thread(&ws::Server::handleClient, this, client.value);
			thread.detach();
		}
	}
}

auto ws::Server::handleClient(TcpSocket client) -> void {
	auto result = Http::parseMessage(client);
	if(result.fail()) {
		std::cerr << result.reason() << '\n';
		return;
	}

	auto it = result.value.headers.find("Sec-WebSocket-Key");
	if(it == result.value.headers.end()) {
		std::cerr << "Nu-uh\n";
		return;
	}

	const std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

	std::string acceptUnhashed = it->second + magic;
	auto hash = sha1::hash(acceptUnhashed);
	auto hashAsView = std::string_view(reinterpret_cast<const char*>(hash.data()), hash.size());
	auto base64Hash = base64::encode(hashAsView);

	std::string response = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n";
	response += "Sec-Websocket-Accept: " + base64Hash + "\n\n";
	client.write(response);

	clientsMutex.lock();
	clients.insert(client);
	clientsMutex.unlock();

	while(true) {
		auto readFrame = decode(client);
		if(readFrame.fail()) {
			client.close();
			clientsMutex.lock();
			clients.erase(client);
			clientsMutex.unlock();
			break;
		}

		switch(readFrame.value.op) {
			case Frame::Opcode::Close:
				client.close();
				clientsMutex.lock();
				clients.erase(client);
				clientsMutex.unlock();
				return;
			default:
				auto view = std::string_view(reinterpret_cast<const char*>(readFrame.value.payload.data()),
						readFrame.value.payload.size());
				std::cerr << "Unhandled op: " << (int)readFrame.value.op << '\n';
				std::cerr << "Payload (length: " << readFrame.value.payload.size()
					<< "): " << view << '\n';
				client.close();
				clientsMutex.lock();
				clients.erase(client);
				clientsMutex.unlock();
				return;
		}
	}
}
