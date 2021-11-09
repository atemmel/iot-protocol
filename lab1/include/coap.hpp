#pragma once
#include <cstdint>
#include <string>

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

	enum OptionType : uint32_t {
		IfMatch = 1,
		UriHost = 3,
		ETag = 4,
		IfNoneMatch = 5,
		UriPort = 7,
		LocationPath = 8,
		UriPath = 11,
		ContentFormat = 12,
		MaxAge = 14,
		UriQuery = 15,
		Accept = 17,
		LocationQuery = 20,
		Size2 = 28,
		ProxyUri = 35,
		ProxyScheme = 39,
		Size1 = 60,
	};

	enum ContentFormats : uint32_t {
		Text = 0,
		LinkFormat = 40,
		Xml = 41,
		OctetStream = 42,
		Exi = 47,
		Json = 50,
		Cbor = 60,
	};

	struct Option {
		uint32_t uint16;
		uint32_t uint32;
		std::string string;
		OptionType type;

		auto isUint16() const -> bool;
		auto isUint32() const -> bool;
		auto isString() const -> bool;
	};

	struct Message {
		Type type;
		Code code;
		uint32_t id;
		Bytes tokens;
		std::vector<Option> options;
		Bytes payload;
	};

	static auto toString(Type type) -> std::string_view;
	static auto toString(Code code) -> std::string_view;
	static auto toString(OptionType optionType) -> std::string_view;
	static auto toString(ContentFormats format) -> std::string_view;

	static auto encode(const Message& message) -> Bytes;
	static auto decode(BytesView bytes) -> std::tuple<Message, Error>;

private:
	static auto decodeHeader(Message& message, BytesView bytes, size_t& offset) -> Error;

	struct HeaderRepresentation {
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

		static auto fromMessage(const Message& message) -> HeaderRepresentation;

		auto setVersion(uint32_t version) -> void;

		auto setType(uint32_t type) -> void;

		auto setTokenLength(uint32_t length) -> void;

		auto setCode(uint32_t code) -> void;

		auto setMessageId(uint32_t id) -> void;

		auto getVersion() const -> uint32_t;

		auto getType() const -> uint32_t;

		auto getTokenLength() const -> uint32_t;

		auto getCode() const -> uint32_t;

		auto getMessageId() const -> uint32_t;
	};

	struct OptionRepresentation {
	private:
		constexpr static uint8_t typeMask = 0xF0;
		constexpr static uint8_t typeShift = 4;

		constexpr static uint8_t lengthMask = 0x0F;
		constexpr static uint8_t lengthShift = 0;

	public:
		uint8_t data = 0;

		static auto fromOption(const Option& option) -> OptionRepresentation;

		auto setType(uint8_t type) -> void;

		auto setLength(uint8_t length) -> void;

		auto getType()  const-> uint8_t;

		auto getLength() const -> uint8_t;
	};
};

std::ostream& operator<<(std::ostream& os, const Coap::Message& message);
