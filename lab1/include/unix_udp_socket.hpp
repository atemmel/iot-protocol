#pragma once
#include <tuple>

#include "common.hpp"
#include "error.hpp"

class UnixUdpSocket {
public:
	static auto create() -> std::tuple<UnixUdpSocket, Error>;

	auto connect(std::string_view address, uint16_t port) -> Error;
	auto read(size_t howManyBytes) -> std::tuple<Bytes, Error>;
	auto readUntil(Byte thisByte) -> std::tuple<Bytes, Error>;
	auto write(const BytesView bytes) -> std::tuple<size_t, Error>;
private:
	int fd;
};
