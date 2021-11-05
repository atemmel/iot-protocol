#include <fstream>
#include <iomanip>
#include <iostream>
#include "coap.hpp"
#include "unix_tcp_socket.hpp"
#include "unix_udp_socket.hpp"

auto main() -> int {
	auto [socket, err] = UnixUdpSocket::create();
	validate(err);
	//err = socket.connect("coap.me", 5683);
	err = socket.connect("127.0.0.1", 5683);
	validate(err);

	Coap::HeaderRepresentation header;
	header.setVersion(1);
	header.setType(Coap::Type::Confirmable);
	header.setCode(Coap::Code::Get);
	header.setTokenLength(0);	// AMOUNT of tokens, not amount of bytes that belong to tokens
	header.setMessageId(1234);

	//std::string_view uri = "test";
	std::string_view uri = "basic";
	Coap::OptionRepresentation option;
	option.setType(Coap::OptionType::UriPath); // URI
	option.setLength(uri.length());

	auto headerAsBytes = AsBytes(header);
	auto optionAsBytes = AsBytes(option);
	auto uriAsBytes = BytesView(uri);

	Bytes message;
	message.reserve(headerAsBytes.size() + optionAsBytes.size() + uriAsBytes.size());
	message.insert(message.end(), headerAsBytes.begin(), headerAsBytes.end());
	message.insert(message.end(), optionAsBytes.begin(), optionAsBytes.end());
	message.insert(message.end(), uriAsBytes.begin(), uriAsBytes.end());
	auto [len, writeError] = socket.write(message);
	validate(writeError);

	Coap::HeaderRepresentation readHeader;
	auto [bytes, readError] = socket.read(1024);
	validate(writeError);

	auto headerBytes = BytesView(bytes.begin(), bytes.begin() + sizeof(Coap::HeaderRepresentation));
	auto [responseHeader, headerError] = fromBytes<Coap::HeaderRepresentation>(headerBytes);
	validate(headerError);

	auto optionBytes = BytesView(bytes.begin() + sizeof(Coap::HeaderRepresentation),
			bytes.begin() + sizeof(Coap::HeaderRepresentation) + sizeof(Coap::OptionRepresentation));
	auto [responseOption, optionError] = fromBytes<Coap::OptionRepresentation>(optionBytes);
	validate(optionError);

	std::cout << std::dec << '\n'
		<< "Version: " << responseHeader.getVersion() << '\n'
		<< "Code: " << responseHeader.getCode() << '\n'
		<< "Type: " << responseHeader.getType()  << '\n'
		<< "Length: " << responseHeader.getTokenLength()  << '\n'
		<< "Message id: " << responseHeader.getMessageId() << '\n'
		<< '\n';

	std::cout 
		<< "Type: " << static_cast<int>(responseOption.getType()) << '\n'
		<< "Length: " << static_cast<int>(responseOption.getLength()) << '\n';

	if(responseOption.getType() == Coap::OptionType::UriQuery) {
		auto remainingBytes = BytesView(bytes.begin() + sizeof(Coap::HeaderRepresentation) + sizeof(Coap::OptionRepresentation),
			bytes.begin() + sizeof(Coap::HeaderRepresentation) + sizeof(Coap::OptionRepresentation) + responseOption.getLength());

		std::string view(remainingBytes.begin(), remainingBytes.end());
		std::cout << view << '\n';
	}
}
