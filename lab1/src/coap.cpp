#include "coap.hpp"

auto Coap::Option::isInteger() const -> bool {
	switch(type) {
		case Coap::OptionType::UriPort:
		case Coap::OptionType::ContentFormat:
		case Coap::OptionType::MaxAge:
		case Coap::OptionType::Accept:
		case Coap::OptionType::Size2:
		case Coap::OptionType::Size1:
			return true;
		default:
			return false;
	}
}

auto Coap::Option::isString() const -> bool {
	switch(type) {
		case Coap::OptionType::UriHost:
		case Coap::OptionType::LocationPath:
		case Coap::OptionType::UriPath:
		case Coap::OptionType::UriQuery:
		case Coap::OptionType::LocationQuery:
		case Coap::OptionType::ProxyUri:
		case Coap::OptionType::ProxyScheme:
			return true;
		default:
			return false;
	}
}

auto Coap::encode(const Message& message) -> Bytes {
	auto header = HeaderRepresentation::fromMessage(message);
	std::vector<OptionRepresentation> options(message.options.size());

	uint32_t sumOptionsDataSize = 0;
	for(size_t i = 0; i < message.options.size(); i++) {
		options[i] = OptionRepresentation::fromOption(message.options[i]);
		sumOptionsDataSize += options[i].getLength();
	}

	auto sumBytes = sizeof(header) + (options.size() * sizeof(Option))
		+ sumOptionsDataSize + message.tokens.size() + message.payload.size() + 1;

	Bytes bytes;
	bytes.reserve(sumBytes);

	// Header
	{
		auto headerAsBytes = AsBytes(header);
		bytes.insert(bytes.end(), headerAsBytes.begin(), headerAsBytes.end());
	}

	// Tokens
	bytes.insert(bytes.end(), message.tokens.begin(), message.tokens.end());

	// Options
	for(auto option : options) {
		auto optionAsBytes = AsBytes(option);
		bytes.insert(bytes.end(), optionAsBytes.begin(), optionAsBytes.end());
	}

	// Full byte
	bytes.insert(bytes.end(), 0xFF);

	// Payload
	bytes.insert(bytes.end(), message.payload.begin(), message.payload.end());

	return bytes;
}

auto Coap::decode(BytesView bytes) -> std::tuple<Message, Error> {
	size_t offset = sizeof(HeaderRepresentation);
	Message message;

	if(bytes.size() < offset) {
		return {
			message,
			"Bytes did not contain a header",
		};
	}

	auto headerBytes = BytesView(bytes.begin(), bytes.begin() + offset);
	auto [header, headerErr] = fromBytes<HeaderRepresentation>(headerBytes);

	if(headerErr) {
		return {
			message,
			headerErr,
		};
	}

	uint32_t version = header.getVersion();
	if(version != 1) {
		return {
			message,
			"Message had a bad version ( != 1)",
		};
	}

	message.type = static_cast<Coap::Type>(header.getType());
	uint32_t tokenLength = header.getTokenLength();
	message.code = static_cast<Coap::Code>(header.getCode());
	message.id = header.getMessageId();

	auto tokensBegin = bytes.begin();
	if(bytes.size() < offset + tokenLength) {
		return {
			message,
			"Message contained too few tokens compared to what the message header specified",
		};
	}

	offset += tokenLength;
	auto tokensEnd = bytes.begin() + offset;
	message.tokens.assign(tokensBegin, tokensEnd);

	while(offset < bytes.size()) {
		if(bytes[offset] == 0xff) {
			offset++;
			break;
		}

		auto optionBytes = BytesView(bytes.begin() + offset,
			bytes.begin() + offset + sizeof(OptionRepresentation));
		auto [optionRep, err] = fromBytes<OptionRepresentation>(optionBytes);
		if(err) {
			return {
				message,
				err,
			};
		}

		Option option;
		option.type = static_cast<Coap::OptionType>(optionRep.getType());

		auto valueBegin = bytes.begin() + offset;
		offset += optionRep.getLength();
		auto valueEnd = bytes.begin() + offset;

		if(option.isInteger()) {
			auto valueBytes = BytesView(valueBegin, valueEnd);
			auto [value, err] = fromBytes<uint32_t>(valueBytes);
			if(err) {
				return {
					message,
					err,
				};
			}

			option.integer = value;
		} else if(option.isString()) {
			option.string.assign(valueBegin, valueEnd);
		}

		message.options.push_back(option);
		offset++;
	}

	if(bytes.size() < offset) {
		return {
			message,
			nullptr,
		};
	}

	message.payload.assign(bytes.begin() + offset, bytes.end());
	return {
		message,
		nullptr
	};
}

auto Coap::HeaderRepresentation::fromMessage(const Message& message) -> HeaderRepresentation {
	HeaderRepresentation header;
	header.setVersion(1);
	header.setType(message.type);
	header.setTokenLength(message.tokens.size());
	header.setCode(message.code);
	header.setMessageId(message.id);
	return header;
}

auto Coap::HeaderRepresentation::setVersion(uint32_t version) -> void {
	setIntSegment(data, version, versionMask, versionShift);
}

auto Coap::HeaderRepresentation::setType(uint32_t type) -> void {
	setIntSegment(data, type, typeMask, typeShift);
}

auto Coap::HeaderRepresentation::setTokenLength(uint32_t length) -> void {
	setIntSegment(data, length, tokenLengthMask, tokenLengthShift);
}

auto Coap::HeaderRepresentation::setCode(uint32_t code) -> void {
	setIntSegment(data, code, codeMask, codeShift);
}

auto Coap::HeaderRepresentation::setMessageId(uint32_t id) -> void {
	setIntSegment(data, id, messageIdMask, messageIdShift);
}

auto Coap::OptionRepresentation::fromOption(const Option& option) -> OptionRepresentation {
	OptionRepresentation optionRep;
	optionRep.setType(option.type);
	if(option.isInteger()) {
		optionRep.setLength(sizeof(uint32_t));
	} else if(option.isString()) {
		optionRep.setLength(option.string.size());
	}
	return optionRep;
}

auto Coap::HeaderRepresentation::getVersion() const -> uint32_t {
	return getIntSegment(data, versionMask, versionShift);
}
auto Coap::HeaderRepresentation::getType() const -> uint32_t {
	return getIntSegment(data, typeMask, typeShift);
}
auto Coap::HeaderRepresentation::getTokenLength() const -> uint32_t {
	return getIntSegment(data, tokenLengthMask, tokenLengthShift);
}
auto Coap::HeaderRepresentation::getCode() const -> uint32_t {
	return getIntSegment(data, codeMask, codeShift);
}
auto Coap::HeaderRepresentation::getMessageId() const -> uint32_t {
	return getIntSegment(data, messageIdMask, messageIdShift);
}

auto Coap::OptionRepresentation::setType(uint8_t type) -> void {
	setIntSegment(data, type, typeMask, typeShift);
}

auto Coap::OptionRepresentation::setLength(uint8_t length) -> void {
	setIntSegment(data, length, lengthMask, lengthShift);
}

auto Coap::OptionRepresentation::getType()  const-> uint8_t {
	return getIntSegment(data, typeMask, typeShift);
}

auto Coap::OptionRepresentation::getLength() const -> uint8_t {
	return getIntSegment(data, lengthMask, lengthShift);
}
