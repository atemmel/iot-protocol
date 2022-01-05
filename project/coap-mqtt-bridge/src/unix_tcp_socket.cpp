#include "unix_tcp_socket.hpp"

#include "unix_dns_lookup.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
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

	auto [addresses, err] = dnsLookup(address, port);
	if(err) {
		return err;
	}

	for(const std::string& ip : addresses) {
		inet_pton(AF_INET, ip.c_str(), &hint.sin_addr);

		const auto hintAsParam = reinterpret_cast<const sockaddr*>(&hint);
		int result = ::connect(fd, hintAsParam, sizeof hint);
		if(result >= 0) {
			return nullptr;
		}	
	}

	return "Could not connect to address/port combination";
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
