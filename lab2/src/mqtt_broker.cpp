#include "mqtt_broker.hpp"

#include "unix_tcp_socket.hpp"

auto MqttBroker::serve() -> void {
	auto [listener, err ] = UnixTcpSocket::create();
	validate(err);

	//https://mqtt.org/faq/
	err = listener.listen(1883);
	validate(err);

	while(true) {
		auto [client, err] = listener.accept();
		if(err) {
			std::cerr << err.string() << '\n';
		} else {
			//client moment???
		}
	}
}
