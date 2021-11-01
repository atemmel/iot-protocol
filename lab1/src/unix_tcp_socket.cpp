#include "unix_tcp_socket.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <cstring>
#include <unistd.h>

auto UnixTcpSocket::create() -> std::tuple<UnixTcpSocket, Error> {
	UnixTcpSocket tcpSocket;
	tcpSocket.fd = socket(AF_INET, SOCK_STREAM, 0);
	if(tcpSocket.fd < 0) {
		return {
			tcpSocket,
			"Could not create socket",
		};
	}

	return {
		tcpSocket, 
		nullptr,
	};
}

auto UnixTcpSocket::connect(std::string_view address, uint16_t port) -> Error {
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);

	auto [addresses, err] = lookup(address, port);
	if(err) {
		return err;
	}

	for(const auto& address : addresses) {
		inet_pton(AF_INET, address.data(), &hint.sin_addr);

		const auto hintAsParam = reinterpret_cast<const sockaddr*>(&hint);
		int result = ::connect(fd, hintAsParam, sizeof hint);
		if(result >= 0) {
			return nullptr;
		}	
	}

	return "Could not connect to address/port combination";
}

auto UnixTcpSocket::lookup(std::string_view address, uint16_t port) -> std::tuple<std::vector<std::string>, Error> {
	addrinfo hint;
	addrinfo* info;

	std::memset(&hint, 0, sizeof hint);
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_flags = AI_PASSIVE;
	
	auto string = std::to_string(port);
	auto status = getaddrinfo(address.data(), string.c_str(), &hint, &info);
	if(status < 0) {
		return {
			{}, 
			"Could not lookup address/port combination",
		};
	}

	std::vector<std::string> addresses;
	std::string str;
	str.resize(INET6_ADDRSTRLEN);

	auto ptr = info;
	while(ptr) {
		void* addr = nullptr;
		if(ptr->ai_family == AF_INET) {
			auto ipv4 = reinterpret_cast<sockaddr_in*>(ptr->ai_addr);
			addr = reinterpret_cast<void*>(&ipv4->sin_addr);
		} else if(ptr->ai_family == AF_INET6) {
			auto ipv6 = reinterpret_cast<sockaddr_in6*>(ptr->ai_addr);
			addr = reinterpret_cast<void*>(&ipv6->sin6_addr);
		} else {
			return {
				{},
				"Unrecognized internet family found",
			};
		}

		inet_ntop(ptr->ai_family, addr, str.data(), str.size());
		addresses.push_back(str);
		ptr = ptr->ai_next;
	}

	freeaddrinfo(info);

	return {
		addresses,
		nullptr,
	};
}

auto UnixTcpSocket::read(size_t howManyBytes) -> std::tuple<Bytes, Error> {
	Bytes bytes(howManyBytes);
	auto result = ::read(fd, bytes.data(), bytes.size());
	if(result < 0) {
		return {
			{},
			"Reading from socket failed",
		};
	}
	return {
		bytes,
		nullptr,
	};
}

auto UnixTcpSocket::readUntil(Byte thisByte) -> std::tuple<Bytes, Error> {
	Bytes bytes;
	bytes.reserve(64);

	Byte byte = 0;
	while(true) {
		auto result = ::read(fd, &byte, 1);
		if(result < 0) {
			return {
				{},
				"Reading from socket failed",
			};
		}

		bytes.push_back(byte);

		if(byte == thisByte) {
			return {
				bytes,
				nullptr,
			};
		}
	}
}

auto UnixTcpSocket::write(const BytesView bytes) -> std::tuple<size_t, Error> {
	auto result = ::write(fd, bytes.data(), bytes.size());
	if(result < 0) {
		return {
			0,
			"Writing to socket failed",
		};
	}
	return {
		result,
		nullptr,
	};
}
