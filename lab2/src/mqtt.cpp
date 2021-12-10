#include "mqtt.hpp"

auto Mqtt::toString(Type type) -> std::string_view {
	switch(type) {
		case Connect:
			return "Connect";
		case Connack:
			return "Connack";
		case Publish:
			return "Publish";
		case Puback:
			return "Puback";
		case Pubrec:
			return "Pubrec";
		case Pubrel:
			return "Pubrel";
		case Pubcomp:
			return "Pubcomp";
		case Subscribe:
			return "Subscribe";
		case Suback:
			return "Suback";
		case Unsubscribe:
			return "Unsubscribe";
		case Unsuback:
			return "Unsuback";
		case Pingreq:
			return "Pingreq";
		case Pingresp:
			return "Pingresp";
		case Disconnect:
			return "Disconnect";
	}
	return "Unrecognized";
}

auto Mqtt::toString(QosLevel level) -> std::string_view {
	switch(level) {
		case Lv0:
			return "Level 0";
		case Lv1:
			return "Level 1";
		case Lv2:
			return "Level 2";
	}
	return "Unrecognized";
}

auto Mqtt::decode(UnixTcpSocket client) -> std::tuple<Message, Error> {
	Message message;

	auto [bytes, err] = client.read(sizeof(HeaderRepresentation));
	if(err) {
		return {
			message,
			err,
		};
	}

	if(bytes.size() < sizeof(HeaderRepresentation)) {
		return {
			message, 
			"Bytes did not contain a header",
		};
	}

	auto headerBytes = BytesView(bytes.begin(), bytes.begin() + sizeof(HeaderRepresentation));
	auto [header, convErr] = fromLittleEndianBytes<HeaderRepresentation>(headerBytes);
	if(convErr) {
		return {
			message,
			convErr,
		};
	}

	message.type = static_cast<Type>(header.getType());
	message.level = static_cast<QosLevel>(header.getQos());
	message.duplicate = header.getDuplicate();
	message.retain = header.getRetain();

	// accumulate length
	size_t remainingLength = 0;
	for(int i = 0; i < 4; i++) {
		std::tie(bytes, err) = client.read(sizeof(Byte));
		if(err) {
			return {
				message,
				err,
			};
		}
		uint8_t byte = bytes[0];
		remainingLength += byte & 0b11111110;
		if((remainingLength & 0b00000001) == 0) {
			break;
		}
	}

	std::tie(bytes, err) = client.read(remainingLength);
	if(err) {
		return {
			message,
			err,
		};
	}

	BytesView remainder(bytes);
	
	
	switch(message.type) {
		case Connect:
			std::tie(message.content, err) = decodeConnect(remainder);
			if(err) {
				return {
					message,
					err,
				};
			}
			break;
		case Connack:
			break;
		case Publish:
			break;
		case Pubrec:
			break;
		case Pubrel:
			break;
		case Pubcomp:
			break;
		case Puback:
			break;
		case Subscribe:
			break;
		case Suback:
			break;
		case Unsubscribe:
			break;
		case Unsuback:
			break;
		case Pingreq:
			break;
		case Pingresp:
			break;
		case Disconnect:
			break;
	}

	return {
		message,
		nullptr,
	};
}

auto Mqtt::encode(const Mqtt::Message& message) -> Bytes {
	Bytes bytes;

	size_t finalSize = sizeof(HeaderRepresentation);
	if(std::get_if<Mqtt::ConnackHeader>(&message.content)) {
		finalSize += 1 + 2;
	} else {
		validate("Unknown message type");
	}

	bytes.reserve(finalSize);

	auto header = HeaderRepresentation::fromMessage(message);
	bytes.insert(bytes.end(), header.data);

	if(auto content = std::get_if<Mqtt::ConnackHeader>(&message.content); content) {
		bytes.insert(bytes.end(), {2, 0});
		bytes.insert(bytes.end(), content->code);
	}

	return bytes;
}

auto Mqtt::decodeConnect(BytesView bytes) -> std::tuple<ConnectHeader, Error> {
	ConnectHeader header;
	if(bytes.size() < 2) {
		return {
			header,
			"Bytes not long enough to fit header",
		};
	}

	auto varHeaderBytes = BytesView(bytes.begin(), bytes.begin() + 2);
	auto [varHeaderLength, varHeaderErr] = fromBigEndianBytes<uint16_t>(varHeaderBytes);
	if(varHeaderErr) {
		return {
			header,
			varHeaderErr,
		};
	}

	header.protocol.resize(varHeaderLength);
	std::copy(bytes.begin() + 2, bytes.begin() + 2 + varHeaderLength, header.protocol.begin());
	
	size_t offset = 2 + varHeaderLength;
	
	if(bytes.size() < offset + 1) {
		return {
			header,
			"Bytes not long enough to fit protocol version",
		};
	}

	header.version = bytes[offset];
	offset++;

	if(bytes.size() < offset + 1) {
		return {
			header,
			"Bytes not long enough to fit flag data",
		};
	}

	header.flags = bytes[offset];
	offset++;

	if(bytes.size() < offset + 2) {
		return {
			header,
			"Bytes not long enough to fit keep alive data",
		};
	}

	BytesView keepAliveBytes(bytes.begin() + offset, bytes.begin() + offset + 2);
	auto [keepAlive, keepAliveErr] = fromBigEndianBytes<uint16_t>(keepAliveBytes);
	if(keepAliveErr) {
		return {
			header,
			keepAliveErr,
		};
	}
	header.keepAlive = keepAlive;

	offset += 2;
	if(bytes.size() < offset + 2) {
		return {
			header,
			"Bytes not long enough to fit payload length data",
		};
	}

	BytesView payloadLengthBytes(bytes.begin() + offset, bytes.begin() + offset + 2);
	auto [payloadLength, payloadLengthErr] = fromBigEndianBytes<uint16_t>(payloadLengthBytes);
	if(payloadLengthErr) {
		return {
			header,
			payloadLengthErr,
		};
	}

	offset += 2;
	if(bytes.size() != offset + payloadLength) {
		return {
			header,
			"Bytes not as long as specified by payload length data",
		};
	}

	header.identifier.resize(payloadLength);
	std::copy(bytes.begin() + offset, bytes.end(), header.identifier.begin());

	return {
		header,
		nullptr,
	};
}

auto Mqtt::HeaderRepresentation::fromMessage(const Message& message) -> HeaderRepresentation {
	HeaderRepresentation header;
	header.setType(static_cast<uint8_t>(message.type));
	header.setDuplicate(static_cast<uint8_t>(message.duplicate));
	header.setQos(static_cast<uint8_t>(message.level));
	header.setRetain(static_cast<uint8_t>(message.retain));
	return header;
}

auto Mqtt::HeaderRepresentation::setType(uint8_t type) -> void {
	setIntSegment(data, type, typeMask, typeShift);
}

auto Mqtt::HeaderRepresentation::setDuplicate(uint8_t duplicate) -> void {
	setIntSegment(data, duplicate, duplicateMask, duplicateShift);
}

auto Mqtt::HeaderRepresentation::setQos(uint8_t qos) -> void {
	setIntSegment(data, qos, qosMask, qosShift);
}

auto Mqtt::HeaderRepresentation::setRetain(uint8_t retain) -> void {
	setIntSegment(data, retain, retainMask, retainShift);
}

auto Mqtt::HeaderRepresentation::getType() const -> uint8_t {
	return getIntSegment(data, typeMask, typeShift);
}

auto Mqtt::HeaderRepresentation::getDuplicate() const -> uint8_t {
	return getIntSegment(data, duplicateMask, duplicateShift);
}

auto Mqtt::HeaderRepresentation::getQos() const -> uint8_t {
	return getIntSegment(data, qosMask, qosShift);
}

auto Mqtt::HeaderRepresentation::getRetain() const -> uint8_t {
	return getIntSegment(data, retainMask, retainShift);
}

std::ostream& operator<<(std::ostream& os, const Mqtt::Message& message) {
	os << "Type: " << Mqtt::toString(message.type) 
		<< ", QoS: " << Mqtt::toString(message.level) 
		<< ", Duplicate: " << (message.duplicate ? "true" : "false")
		<< ", Retain: " << (message.retain ? "true" : "false");

	if(auto connect = std::get_if<Mqtt::ConnectHeader>(&message.content); connect) {
		os << ", Protocol: " << connect->protocol 
			<< ", Identifier: " << connect->identifier
			<< ", Keep-alive: " << connect->keepAlive
			<< ", Version: " << static_cast<int>(connect->version);
	}

	return os;
}
