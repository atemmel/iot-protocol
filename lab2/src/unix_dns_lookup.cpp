#include "unix_dns_lookup.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <netdb.h>
#include <unistd.h>

auto dnsLookup(std::string_view address, uint16_t port) -> std::tuple<std::vector<std::string>, Error> {
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

