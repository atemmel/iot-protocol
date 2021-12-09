#include "mqtt.hpp"

auto Mqtt::HeaderRepresentation::fromMessage(const Message& message) -> HeaderRepresentation {

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
