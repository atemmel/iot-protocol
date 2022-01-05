#include "boilerplate.hpp"

#include <memory>
#include <mqtt_client_cpp.hpp>
#include <mqtt/tcp_endpoint.hpp>

using Wtf = std::shared_ptr<mqtt::callable_overlay<mqtt::sync_client<mqtt::tcp_endpoint<as::ip::tcp::socket, as::io_context::strand>>>>;
std::function<MqttClient::PublishResponse(const std::string&)> MqttClient::publishHandler;
std::string MqttClient::address;
std::string MqttClient::port;

namespace MqttClient {

auto setAddressAndPort(const std::string& address, const std::string& port) -> void {
	MqttClient::address = address;
	MqttClient::port = port;
}

auto serve() -> void {
	Wtf c;
	boost::asio::io_context ioc;

	std::uint16_t pid_sub1;
	std::uint16_t pid_sub2;
	MQTT_NS::setup_log();
    c = MQTT_NS::make_sync_client(ioc, address, port);
    using packet_id_t = typename std::remove_reference_t<decltype(*c)>::packet_id_t;
    c->set_client_id("coap-mqtt-bridge");
    c->set_clean_session(true);

    c->set_connack_handler(
        [&c, &pid_sub1, &pid_sub2]
        (bool sp, MQTT_NS::connect_return_code connack_return_code){
            std::cout << "Connack handler called" << std::endl;
            std::cout << "Session Present: " << std::boolalpha << sp << std::endl;
            std::cout << "Connack Return Code: "
                      << MQTT_NS::connect_return_code_to_str(connack_return_code) << std::endl;
            if (connack_return_code == MQTT_NS::connect_return_code::accepted) {
                pid_sub1 = c->subscribe("req/mem", MQTT_NS::qos::at_most_once);
            }
			if(connack_return_code == MQTT_NS::connect_return_code::accepted) {
				pid_sub2 = c->subscribe("req/cpu", MQTT_NS::qos::at_most_once);
			}
            return true;
        });
    c->set_publish_handler(
        [&]
        (MQTT_NS::optional<packet_id_t> packet_id,
         MQTT_NS::publish_options pubopts,
         MQTT_NS::buffer topic_name,
         MQTT_NS::buffer contents){
            std::cout << "publish received."
                      << " dup: "    << pubopts.get_dup()
                      << " qos: "    << pubopts.get_qos()
                      << " retain: " << pubopts.get_retain() << std::endl;
            if (packet_id)
                std::cout << "packet_id: " << *packet_id << std::endl;
            std::cout << "topic_name: " << topic_name << std::endl;
            std::cout << "contents: " << contents << std::endl;

			auto response = publishHandler(std::string(topic_name));
			std::cerr << response.topic << ' ' << response.response << ' ' << response.send << '\n';
			if(response.send) {
				c->publish(response.topic, response.response);
			}
			std::cerr << "Done sending!\n";
            return true;
        });

    // Connect
    c->connect();

    ioc.run();
	std::cerr << "Bye bye!\n";
}

};
