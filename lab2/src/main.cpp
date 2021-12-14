#include "mqtt_broker.hpp"

auto main() -> int {
	MqttBroker().serve();
}
