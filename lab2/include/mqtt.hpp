#pragma once
#include <cstdint>
#include <variant>

#include "common.hpp"
#include "unix_tcp_socket.hpp"

//https://openlabpro.com/guide/mqtt-packet-format/

// 2 1st bytes = header
// first 4 bits = message type
// 5th bit = DUP flag
// 6th and 7th bit - QoS Level
// 8th bit RETAIN flag
// 2nd byte = Remaining length, 1-4 bytes
// the rest is payload

class Mqtt {
public:
	enum Type : uint32_t {
		Connect = 1,		// Connection request
		Connack = 2,		// Connection acknowlegement
		Publish = 3,		// Publish message
		Puback = 4,			// Publish acknowlegement
		Pubrec = 5,			// Publish recieved (QoS Lv2 part 1)
		Pubrel = 6,			// Publish release (QoS Lv2 part 2)
		Pubcomp = 7,		// Publish complete (QoS Lv2 part 3)
		// Don't forget to like and
		Subscribe = 8,		// Subscribe request
		Suback = 9,			//Subscribe acknowlegement
		Unsubscribe = 10,	// Unsubscribe request
		Unsuback = 11,		// Unsubscribe acknowlegement
		Pingreq = 12,		// Ping request
		Pingresp = 13,		// Ping acknowlegement
		Disconnect = 14,	// Disconnect notification
	};

	enum QosLevel : uint32_t {
		Lv0 = 0,
		Lv1 = 1,
		Lv2 = 2,
	};

	struct ConnectHeader {
		std::string protocol;
		std::string identifier;
		uint16_t keepAlive;
		uint8_t version;
		uint8_t flags;
	};

	struct ConnackHeader {
		uint8_t code;
	};

	struct PublishHeader {
		std::string topic;
		std::string payload;
		uint16_t id;
	};

	struct SubscribeHeader {
		std::vector<std::string> topics;
		std::vector<QosLevel> levels;
		uint16_t id;
	};

	struct SubackHeader {
		std::vector<Byte> payload;
		uint16_t id;
	};

	struct UnsubscribeHeader {
		std::vector<std::string> topics;
		uint16_t id;
	};

	struct Message {
		Type type;
		QosLevel level;
		bool duplicate;
		bool retain;

		std::variant<ConnectHeader, ConnackHeader, PublishHeader, SubscribeHeader, SubackHeader, UnsubscribeHeader> content;
	};

	static auto toString(Type type) -> std::string_view;
	static auto toString(QosLevel level) -> std::string_view;

	static auto decode(UnixTcpSocket client) -> std::tuple<Message, Bytes, Error>;
	static auto encode(const Mqtt::Message& message) -> Bytes;

private:
	static auto decodeConnect(BytesView bytes) -> std::tuple<ConnectHeader, Error>;
	static auto decodePublish(BytesView bytes, QosLevel level) -> std::tuple<PublishHeader, Error>;
	static auto decodeSubscribe(BytesView bytes) -> std::tuple<SubscribeHeader, Error>;
	static auto decodeUnsubscribe(BytesView bytes) -> std::tuple<UnsubscribeHeader, Error>;

	struct HeaderRepresentation {
	private:
		constexpr static uint8_t typeMask = 0b11110000;
		constexpr static uint8_t typeShift = 4;

		constexpr static uint8_t duplicateMask = 0b00001000;
		constexpr static uint8_t duplicateShift = 3;

		constexpr static uint8_t qosMask = 0b00000110;
		constexpr static uint8_t qosShift = 1;

		constexpr static uint8_t retainMask = 0b00000001;
		constexpr static uint8_t retainShift = 0;
	public:
		uint8_t data;

		static auto fromMessage(const Message& message) -> HeaderRepresentation;

		auto setType(uint8_t type) -> void;
		auto setDuplicate(uint8_t duplicate) -> void;
		auto setQos(uint8_t qos) -> void;
		auto setRetain(uint8_t retain) -> void;

		auto getType() const -> uint8_t;
		auto getDuplicate() const -> uint8_t;
		auto getQos() const -> uint8_t;
		auto getRetain() const -> uint8_t;
	};


};

std::ostream& operator<<(std::ostream& os, const Mqtt::Message& message);
