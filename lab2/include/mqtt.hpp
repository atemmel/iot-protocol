#pragma once
#include <cstdint>

// 2 1st bytes = header
// first 4 bits = message type
// 5th bit = DUP flag
// 6th and 7th bit - QoS Level
// 8th bit RETAIN flag
// 2nd byte = Remaining length, 1-4 bytes

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

	struct Message {
		Type type;
		QosLevel level;
		bool duplicate;
		bool retain;
	};

private:
};
