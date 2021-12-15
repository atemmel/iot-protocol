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

auto Mqtt::decode(UnixTcpSocket client) -> std::tuple<Message, Bytes, Error> {
	Message message;
	Bytes bytesResult;

	auto [bytes, err] = client.read(sizeof(HeaderRepresentation));
	if(err) {
		return {
			message,
			{},
			err,
		};
	}

	bytesResult.insert(bytesResult.end(), bytes.begin(), bytes.end());

	if(bytes.size() < sizeof(HeaderRepresentation)) {
		return {
			message, 
			{},
			"Bytes did not contain a header",
		};
	}

	auto headerBytes = BytesView(bytes.begin(), bytes.begin() + sizeof(HeaderRepresentation));
	auto [header, convErr] = fromLittleEndianBytes<HeaderRepresentation>(headerBytes);
	if(convErr) {
		return {
			message,
			{},
			convErr,
		};
	}

	message.type = static_cast<Type>(header.getType());
	message.level = static_cast<QosLevel>(header.getQos());
	message.duplicate = header.getDuplicate();
	message.retain = header.getRetain();

	// accumulate length
	size_t remainingLength = 0;
	size_t multiplier = 1;
	Byte byte = 0;

	do {
		std::tie(bytes, err) = client.read(sizeof(Byte));
		if(err) {
			return {
				message,
				{},
				err,
			};
		}

		byte = bytes[0];
		bytesResult.insert(bytesResult.end(), byte);
		remainingLength += (byte & 127) * multiplier;
		multiplier *= 128;
		if(multiplier > 128 * 128 * 128) {
			return {
				message,
				{},
				"Error decoding length",
			};
		}
	} while((byte & 128) != 0);

	std::tie(bytes, err) = client.read(remainingLength);
	if(err) {
		return {
			message,
			{},
			err,
		};
	}

	BytesView remainder(bytes);
	bytesResult.insert(bytesResult.end(), remainder.begin(), remainder.end());
	
	switch(message.type) {
		case Connect:
			std::tie(message.content, err) = decodeConnect(remainder);
			if(err) {
				return {
					message,
					{},
					err,
				};
			}
			break;
		case Connack:
			break;
		case Publish:
			std::tie(message.content, err) = decodePublish(remainder, message.level);
			if(err) {
				return {
					message,
					{},
					err,
				};
			}
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
			std::tie(message.content, err) = decodeSubscribe(remainder);
			if(err) {
				return {
					message,
					{},
					err,
				};
			}
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
		bytesResult,
		nullptr,
	};
}

auto Mqtt::encode(const Mqtt::Message& message) -> Bytes {
	Bytes bytes;

	size_t finalSize = sizeof(HeaderRepresentation);
	if(message.type == Mqtt::Pingresp) {
		finalSize += 1;
	} else if(std::get_if<Mqtt::ConnackHeader>(&message.content)) {
		finalSize += 1 + 2;
	} else if(std::get_if<Mqtt::SubackHeader>(&message.content)) {
		finalSize += 2 + 1;
	} else if(auto content = std::get_if<Mqtt::PublishHeader>(&message.content); content) {
		finalSize += 4 + 2 + 2 + content->payload.size() 
			+ content->topic.size() + 2;
	} else {
		validate("Unknown message type");
	}

	bytes.reserve(finalSize);

	auto header = HeaderRepresentation::fromMessage(message);
	bytes.insert(bytes.end(), header.data);

	if(message.type == Mqtt::Pingresp) {
		bytes.insert(bytes.end(), 0);
	} else if(auto connack = std::get_if<Mqtt::ConnackHeader>(&message.content); connack) {
		bytes.insert(bytes.end(), {2, 0});
		bytes.insert(bytes.end(), connack->code);
	} else if(auto suback = std::get_if<Mqtt::SubackHeader>(&message.content); suback) {
		bytes.insert(bytes.end(), 2 + suback->payload.size());
		auto idBytes = AsBigEndianBytes(suback->id);
		bytes.insert(bytes.end(), idBytes.begin(), idBytes.end());
		bytes.insert(bytes.end(), suback->payload.begin(), suback->payload.end());
	} else if(auto publish = std::get_if<Mqtt::PublishHeader>(&message.content); publish) {
		uint16_t topicLength = publish->topic.size();
		uint16_t payloadLength = publish->payload.size();
		uint32_t totalLength = topicLength + payloadLength + 2;

		do {
			Byte byte = totalLength % 128;
			totalLength /= 128;
			if(totalLength > 0) {
				totalLength |= 128;
			}
			bytes.insert(bytes.end(), byte);
		} while(totalLength > 0);

		auto topicLengthBytes = AsBigEndianBytes(topicLength);
		bytes.insert(bytes.end(), topicLengthBytes.begin(), topicLengthBytes.end());
		bytes.insert(bytes.end(), publish->topic.begin(), publish->topic.end());
		bytes.insert(bytes.end(), publish->payload.begin(), publish->payload.end());
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

auto Mqtt::decodePublish(BytesView bytes, QosLevel level) -> std::tuple<PublishHeader, Error> {
	PublishHeader header;
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

	header.topic.resize(varHeaderLength);
	std::copy(bytes.begin() + 2, bytes.begin() + 2 + varHeaderLength, header.topic.begin());
	
	size_t offset = 2 + varHeaderLength;
	
	if(bytes.size() < offset + 1 && level != QosLevel::Lv0) {
		return {
			header,
			"Bytes not long enough to fit QoS level",
		};
	}

	// id field only present in QoS 1 and 2
	if(level != QosLevel::Lv0) {
		auto idBytes = BytesView(bytes.begin() + offset, bytes.begin() + offset + 2);
		auto [id, idErr] = fromBigEndianBytes<uint16_t>(idBytes);
		if(idErr) {
			return {
				header,
				idErr,
			};
		}

		header.id = id;
		offset += 2;
	}

	header.payload.resize(bytes.size() - offset);
	std::copy(bytes.begin() + offset, bytes.end(), header.payload.begin());

	return {
		header,
		nullptr,
	};
}

auto Mqtt::decodeSubscribe(BytesView bytes) -> std::tuple<SubscribeHeader, Error> {
	SubscribeHeader header;
	std::string topic;
	uint8_t level;
	size_t offset = 0;

	if(bytes.size() < 2) {
		return {
			header,
			"Bytes not long enough to fit message id",
		};
	}

	auto idBytes = BytesView(bytes.begin(), bytes.begin() + 2);
	auto [id, idErr] = fromBigEndianBytes<uint16_t>(idBytes);
	if(idErr) {
		return {
			header,
			idErr,
		};
	}

	header.id = id;
	offset += 2;

	while(offset < bytes.size()) {
		if(bytes.size() < offset + 2) {
			return {
				header,
				"Bytes not long enough to fit topic length",
			};
		}

		auto topicLengthBytes = BytesView(bytes.begin() + offset, bytes.begin() + offset + 2);
		auto [topicLength, topicLengthErr] = fromBigEndianBytes<uint16_t>(topicLengthBytes);
		if(topicLengthErr) {
			return {
				header,
				topicLengthErr,
			};
		}

		offset += 2;

		if(bytes.size() < offset + topicLength) {
			return {
				header,
				"Bytes not enough to fit topic",
			};
		}

		topic.resize(topicLength);
		std::copy(bytes.begin() + offset, bytes.begin() + offset + topicLength, topic.begin());

		offset += topicLength;

		if(bytes.size() <= offset) {
			return {
				header,
				"Bytes not enough to fit QoS level",
			};
		}

		level = bytes[offset];
		offset++;

		header.topics.push_back(topic);
		header.levels.push_back(static_cast<QosLevel>(level));
	}

	return {
		header,
		nullptr,
	};
}

auto Mqtt::decodeUnsubscribe(BytesView bytes) -> std::tuple<Mqtt::UnsubscribeHeader, Error> {
	UnsubscribeHeader header;
	std::string topic;
	size_t offset = 0;

	if(bytes.size() < 2) {
		return {
			header,
			"Bytes not enough to fit id",
		};
	}

	auto idBytes = BytesView(bytes.begin(), bytes.begin() + 2);
	auto [id, err] = fromBigEndianBytes<uint16_t>(idBytes);
	if(err) {
		return {
			header,
			err,
		};
	}

	offset += 2;
	header.id = id;

	while(offset < bytes.size()) {
		if(bytes.size() < offset + 2) {
			return {
				header,
				"Bytes not enough to fit topic length",
			};
		}

		auto topicLengthBytes = BytesView(bytes.begin() + offset, bytes.begin() + offset + 2);
		auto [topicLength, topicLengthErr] = fromBigEndianBytes<uint16_t>(topicLengthBytes);

		if(topicLengthErr) {
			return {
				header,
				topicLengthErr,
			};
		}

		offset += 2;

		if(bytes.size() < offset + topicLength) {
			return {
				header,
				"Bytes not enough to fit topic",
			};
		}

		topic.resize(topicLength);
		std::copy(bytes.begin() + offset, bytes.begin() + offset + topicLength, topic.begin());
		header.topics.push_back(topic);
		offset += topicLength;
	}

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

	if(auto connect = std::get_if<Mqtt::ConnectHeader>(&message.content); connect 
			&& message.type == Mqtt::Connect) {
		os << ", Protocol: " << connect->protocol 
			<< ", Identifier: " << connect->identifier
			<< ", Keep-alive: " << connect->keepAlive
			<< ", Version: " << static_cast<int>(connect->version);
	} else if(auto publish = std::get_if<Mqtt::PublishHeader>(&message.content); publish 
			&& message.type == Mqtt::Publish) {
		os << ", Topic: " << publish->topic
			<< ", Id: " << publish->id
			<< ", Payload: " << publish->payload;
	} else if(auto subscribe = std::get_if<Mqtt::SubscribeHeader>(&message.content); subscribe
			&& message.type == Mqtt::Subscribe) {
		os << ", Topics: ";
		for(auto& topic : subscribe->topics) {
			os << topic << ", ";
		}
		os << " Levels: ";
		for(auto& level : subscribe->levels) {
			os << Mqtt::toString(level) << ", ";
		}
	}

	return os;
}
