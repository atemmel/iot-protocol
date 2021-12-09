#include <iostream>

#include "mqtt_broker.hpp"

auto main() -> int {
	/*
	Mqtt::Message message = {
		.type = Mqtt::Type::Connect,
		.level = Mqtt::QosLevel::Lv0,
		.duplicate = 0,
		.retain = 0,
		.payload = {
			'L',
			'M',
			'A',
			'O',
		},
	};
	*/

	MqttBroker broker;
	broker.serve();
}
