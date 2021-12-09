#pragma once
#include <tuple>
#include <vector>

#include "common.hpp"
#include "error.hpp"

class UnixTcpSocket {
public:
	static auto create() -> std::tuple<UnixTcpSocket, Error>;

	auto connect(std::string_view address, uint16_t port) -> Error;
	auto listen(uint16_t port) -> Error;
	auto accept() -> std::tuple<UnixTcpSocket, Error>;
	auto read(size_t howManyBytes) -> std::tuple<Bytes, Error>;
	auto readUntil(Byte thisByte) -> std::tuple<Bytes, Error>;
	auto write(const BytesView bytes) -> std::tuple<size_t, Error>;
private:
	int fd;
};
