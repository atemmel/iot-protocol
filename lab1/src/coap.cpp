#include "coap.hpp"
#include <iostream>
#include <bitset>

auto Coap::Option::isUint16() const -> bool {
	switch(type) {
		case Coap::OptionType::UriPort:
		case Coap::OptionType::ContentFormat:
		case Coap::OptionType::Accept:
			return true;
		default:
			return false;
	}
}

auto Coap::Option::isUint32() const -> bool {
	switch(type) {
		case Coap::OptionType::MaxAge:
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

auto Coap::toString(Type type) -> std::string_view {
	switch(type) {
		case Confirmable:
			return "Confirmable";
		case NonConfirmable:
			return "NonConfirmable";
		case Acknowledgement:
			return "Acknowledgement";
		case Reset:
			return "Reset";
	}
	return "Unrecognized";
}

auto Coap::toString(Code code) -> std::string_view {
	switch(code) {
		case Empty:
			return "Empty";
		case Get:
			return "Get";
		case Post :
			return "Post";
		case Put :
			return "Put";
		case Delete :
			return "Delete";
		case Created :
			return "Created";
		case Deleted :
			return "Deleted";
		case Valid :
			return "Valid";
		case Changed :
			return "Changed";
		case Content :
			return "Content";
		case Continue :
			return "Continue";
		case BadRequest :
			return "BadRequest";
		case Unauthorized :
			return "Unauthorized";
		case BadOption :
			return "BadOption";
		case Forbidden :
			return "Forbidden";
		case NotFound :
			return "NotFound";
		case MethodNotAllowed :
			return "MethodNotAllowed";
		case NotAcceptable :
			return "NotAcceptable";
		case RequestEntityIncomplete :
			return "RequestEntityIncomplete";
		case PreconditionFailed :
			return "PreconditionFailed";
		case RequestEntityTooLarge :
			return "RequestEntityTooLarge";
		case UnsupportedContentFormat :
			return "UnsupportedContentFormat";
		case InternalServerError :
			return "InternalServerError";
		case NotImplemented :
			return "NotImplemented";
		case BadGateway :
			return "BadGateway";
		case ServiceUnavailable :
			return "ServiceUnavailable";
		case GatewayTimeout :
			return "GatewayTimeout";
		case ProxyingNotSupported:
			return "ProxyingNotSupported";
	}
	return "Unrecognized";
}

auto Coap::toString(OptionType optionType) -> std::string_view {
	switch(optionType) {
		case IfMatch:
			return "IfMatch";
		case UriHost:
			return "UriHost";
		case ETag:
			return "ETag";
		case IfNoneMatch:
			return "IfNoneMatch";
		case UriPort:
			return "UriPort";
		case LocationPath:
			return "LocationPath";
		case UriPath:
			return "UriPath";
		case ContentFormat:
			return "ContentFormat";
		case MaxAge:
			return "MaxAge";
		case UriQuery:
			return "UriQuery";
		case Accept:
			return "Accept";
		case LocationQuery:
			return "LocationQuery";
		case Size2:
			return "Size2";
		case ProxyUri:
			return "ProxyUri";
		case ProxyScheme:
			return "ProxyScheme";
		case Size1:
			return "Size1";
	}
	return "Unrecognized";
}

auto Coap::toString(ContentFormats format) -> std::string_view {
	switch(format) {
		case Text:
			return "Text";
		case LinkFormat:
			return "LinkFormat";
		case Xml:
			return "Xml";
		case OctetStream:
			return "OctetStream";
		case Exi:
			return "Exi";
		case Json:
			return "Json";
		case Cbor:
			return "Cbor";
	}
	return "Unrecognized";
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
	for(size_t i = 0; i < message.options.size(); i++) {
		auto option = options[i];
		auto optionAsBytes = AsBytes(option);
		bytes.insert(bytes.end(), optionAsBytes.begin(), optionAsBytes.end());

		if(message.options[i].isUint32()) {
			auto unsignedAsBytes = AsBytes(message.options[i].uint32);
			bytes.insert(bytes.end(), unsignedAsBytes.begin(), unsignedAsBytes.end());
		} else if(message.options[i].isUint16()) {
			auto unsignedAsBytes = AsBytes(message.options[i].uint16);
			bytes.insert(bytes.end(), unsignedAsBytes.begin(), unsignedAsBytes.end());
		} else if(message.options[i].isString()) {
			bytes.insert(bytes.end(), message.options[i].string.begin(),
				message.options[i].string.end());
		}
	}

	// Optional early exit
	if(message.payload.empty()) {
		return bytes;
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

	if(bytes.size() < offset + tokenLength) {
		return {
			message,
			"Message contained too few tokens compared to what the message header specified",
		};
	}

	auto tokensBegin = bytes.begin() + offset;
	offset += tokenLength;
	auto tokensEnd = bytes.begin() + offset;
	message.tokens.assign(tokensBegin, tokensEnd);

	OptionType prevOption;
	while(offset < bytes.size()) {
		if(bytes[offset] == 0xff) {
			offset++;
			break;
		}

		std::cout << "Option looks like: " << std::bitset<8>(static_cast<int>(bytes[offset])) << '\n';

		auto optionBegin = bytes.begin() + offset;
		offset += sizeof(OptionRepresentation);
		auto optionEnd = bytes.begin() + offset;

		auto optionBytes = BytesView(optionBegin, optionEnd);
		auto [optionRep, err] = fromBytes<OptionRepresentation>(optionBytes);
		if(err) {
			return {
				message,
				err,
			};
		}

		Option option;

		// Fråga någon nisse om det här, makear ingen sense
		if(optionRep.getType() == 0) {
			option.type = prevOption;
		} else {
			option.type = static_cast<Coap::OptionType>(optionRep.getType());
		}

		auto valueBegin = bytes.begin() + offset;
		offset += optionRep.getLength();
		auto valueEnd = bytes.begin() + offset;
		auto valueBytes = BytesView(valueBegin, valueEnd);

		if(valueBytes.size() > 0) {
			if(option.isUint32()) {
				auto [value, err] = fromBytes<uint32_t>(valueBytes);
				if(err) {
					return {
						message,
						err,
					};
				}

				option.uint32 = value;
			} else if(option.isUint16()) {
				auto [value, err] = fromBytes<uint16_t>(valueBytes);
				if(err) {
					return {
						message,
						err,
					};
				}

				option.uint16 = value;
			} else if(option.isString()) {
				option.string.assign(valueBegin, valueEnd);
			}
		} else {
			option.uint16 = 0;
			option.uint32 = 0;
			option.string = "";
		}

		message.options.push_back(option);
		prevOption = option.type;
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
	if(option.isUint32()) {
		optionRep.setLength(sizeof(uint32_t));
	} else if(option.isUint16()) {
		optionRep.setLength(sizeof(uint16_t));
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

std::ostream& operator<<(std::ostream& os, const Coap::Message& message) {
	os << "Type: " << Coap::toString(message.type) 
		<< " Code: " << Coap::toString(message.code) 
		<< " Id: " << message.id
		<< " Tokens: " << message.tokens.size()
		<< " Options: { ";
	for(const auto& opt : message.options) {
		os << "Type: " << Coap::toString(opt.type)
			<< " Value: ";
		if(opt.isUint32()) {
			os << opt.uint32;
		} else if(opt.isUint16()) {
			os << opt.uint16;
		} else if(opt.isString()) {
			os << opt.string;
		} else {
			os << "???";
		}
		os << ' ';
	}
	os << "} ";
	return os << " Payload length: " << message.payload.size();
}
