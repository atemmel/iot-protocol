#pragma once
#include <cstdint>

#include "common.hpp"

class Coap {
public:

	enum Type : uint32_t {
		Confirmable = 0,
		NonConfirmable = 1,
		Acknowledgement = 2,
		Reset = 3,
	};

	enum Code : uint32_t {
		// Methods 
		Empty = 0,
		Get = 1,
		Post = 2,
		Put = 3,
		Delete = 4,

		// Responses
		Created = 65,
		Deleted = 66,
		Valid = 67,
		Changed = 68,
		Content = 69,
		Continue = 95,
		BadRequest = 128,
		Unauthorized = 129,
		BadOption = 130,
		Forbidden = 131,
		NotFound = 132,
		MethodNotAllowed = 133,
		NotAcceptable = 134,
		RequestEntityIncomplete = 136,
		PreconditionFailed = 140,
		RequestEntityTooLarge = 141,
		UnsupportedContentFormat = 143,
		InternalServerError = 160,
		NotImplemented = 161,
		BadGateway = 162,
		ServiceUnavailable = 163,
		GatewayTimeout = 164,
		ProxyingNotSupported = 165,
	};

	struct Header {
	private:
		constexpr static uint32_t versionMask = 0b11000000000000000000000000000000;
		constexpr static uint32_t versionShift = 30u;

		constexpr static uint32_t typeMask = 0b00110000000000000000000000000000;
		constexpr static uint32_t typeShift = 28;

		constexpr static uint32_t tokenLengthMask = 0b00001111000000000000000000000000;
		constexpr static uint32_t tokenLengthShift = 24;

		constexpr static uint32_t codeMask = 0b00000000111111110000000000000000;
		constexpr static uint32_t codeShift = 16;

		constexpr static uint32_t messageIdMask = 0b00000000000000001111111111111111;
		constexpr static uint32_t messageIdShift = 0;
	public:
		uint32_t data = 0;

		auto setVersion(uint32_t version) {
			setIntSegment(data, version, versionMask, versionShift);
		}

		auto setType(uint32_t type) {
			setIntSegment(data, type, typeMask, typeShift);
		}

		auto setTokenLength(uint32_t length) {
			setIntSegment(data, length, tokenLengthMask, tokenLengthShift);
		}

		auto setCode(uint32_t code) {
			setIntSegment(data, code, codeMask, codeShift);
		}

		auto setMessageId(uint32_t id) {
			setIntSegment(data, id, messageIdMask, messageIdShift);
		}

		auto getVersion() const -> uint32_t {
			return getIntSegment(data, versionMask, versionShift);
		}
		auto getType() const -> uint32_t {
			return getIntSegment(data, typeMask, typeShift);
		}
		auto getTokenLength() const -> uint32_t {
			return getIntSegment(data, tokenLengthMask, tokenLengthShift);
		}
		auto getCode() const -> uint32_t {
			return getIntSegment(data, codeMask, codeShift);
		}
		auto getMessageId() const -> uint32_t {
			return getIntSegment(data, messageIdMask, messageIdShift);
		}
	};

	struct Token {
	private:
		constexpr static uint8_t typeMask = 0xF0;
		constexpr static uint8_t typeShift = 4;

		constexpr static uint8_t lengthMask = 0x0F;
		constexpr static uint8_t lengthShift = 0;

	public:
		uint8_t data = 0;

		auto setType(uint8_t type) {
			setIntSegment(data, type, typeMask, typeShift);
		}

		auto setLength(uint8_t length) {
			setIntSegment(data, length, lengthMask, lengthShift);
		}

		auto getType()  const-> uint8_t {
			return getIntSegment(data, typeMask, typeShift);
		}

		auto getLength() const -> uint8_t {
			return getIntSegment(data, lengthMask, lengthShift);
		}
	};
};
