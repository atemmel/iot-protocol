#pragma once
#include <tuple>
#include <vector>

#include "common.hpp"
#include "error.hpp"

class UnixTcpSocket {
public:
	static auto create() -> std::tuple<UnixTcpSocket, Error>;

	auto operator==(UnixTcpSocket other) const -> bool;
	auto operator<(UnixTcpSocket other) const -> bool;

	auto connect(std::string_view address, uint16_t port) -> Error;
	auto listen(uint16_t port) -> Error;
	auto accept() -> std::tuple<UnixTcpSocket, Error>;
	auto read(size_t howManyBytes) const -> std::tuple<Bytes, Error>;
	auto readUntil(Byte thisByte) const -> std::tuple<Bytes, Error>;
	auto write(const BytesView bytes) const -> std::tuple<size_t, Error>;
	auto close() -> void;
private:
	int fd;
};
