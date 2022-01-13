#include <coap3/coap.h>

#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

#include "unix_dns_lookup.hpp"

float cpuUsage = 0.f;
std::mutex cpuUsageMutex;

float memUsage = 0.f;
std::mutex memUsageMutex;

auto readCpuUsage() -> std::vector<size_t> {
	std::vector<size_t> times;
	times.reserve(4);
	std::ifstream procStat("/proc/stat");
	procStat.ignore(5, ' ');
	size_t time;
	while(procStat >> time) {
		times.push_back(time);
	}
	return times;
}

auto parseCpuUsage(size_t& idle, size_t& total) -> bool {
	auto times = readCpuUsage();
	if(times.size() < 4) {
		return false;
	}

	idle = times[3];
	total = 0;
	for(auto time : times) {
		total += time;
	}
	return true;
}

auto refreshCpuUsage() -> void {
	static float previousIdleTime = 0.f;
	static float previousTotalTime = 0.f;

	size_t idle, total;
	if(!parseCpuUsage(idle, total)) {
		return;
	}

	const float idleDelta = idle - previousIdleTime;
	const float totalDelta = total - previousTotalTime;

	//lock
	cpuUsageMutex.lock();
	cpuUsage = 100.0f * (1.0f - idleDelta / totalDelta);
	cpuUsageMutex.unlock();
	//unlock
	
	previousIdleTime = idle;
	previousTotalTime = total;
}

auto readMemUsage(uint64_t& available, uint64_t& total) -> void {
	std::ifstream procMeminfo("/proc/meminfo");
	while(!std::isdigit(procMeminfo.peek())) {
		procMeminfo.ignore();
	}
	procMeminfo >> total;
	// skip till end
	procMeminfo.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	// ignore next line entirely
	procMeminfo.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	while(!std::isdigit(procMeminfo.peek())) {
		procMeminfo.ignore();
	}
	// finito
	procMeminfo >> available;
}

auto refreshMemUsage() -> void {
	uint64_t available, total;
	readMemUsage(available, total);
	const float newUsage = 100.f * (1.0f - static_cast<float>(available) / total);
	memUsageMutex.lock();
	memUsage = newUsage;
	memUsageMutex.unlock();
}

auto die(coap_context_t* ctx, int result) -> void {
	coap_free_context(ctx);
	coap_cleanup();
	std::exit(result);
}

auto main() -> int {
	coap_context_t* ctx = nullptr;
	coap_resource_t* resourceCpu = nullptr;
	coap_resource_t* resourceMem = nullptr;
	coap_endpoint_t* endpoint = nullptr;
	coap_str_const_t* ruriCpu = coap_make_str_const("cpu");
	coap_str_const_t* ruriMem = coap_make_str_const("mem");
	coap_address_t dst;
	coap_startup();

	//resolve address
	resolve_address("127.0.0.1", "5683", &dst);

	ctx = coap_new_context(nullptr);

	if(!ctx || !(endpoint = coap_new_endpoint(ctx, &dst, COAP_PROTO_UDP))) {
		coap_log(LOG_EMERG, "Cannot initialize context\n");
		die(ctx, EXIT_FAILURE);
	}

	resourceCpu = coap_resource_init(ruriCpu, 0);
	resourceMem = coap_resource_init(ruriMem, 0);

	coap_register_handler(resourceCpu, COAP_REQUEST_GET,
			[](auto, auto, const coap_pdu_t* request, auto, 
				coap_pdu_t* response) {
		//coap_show_pdu(LOG_WARNING, request);
		coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
		cpuUsageMutex.lock();
		auto usage = std::to_string(cpuUsage);
		cpuUsageMutex.unlock();
		coap_add_data(response, usage.size(), reinterpret_cast<const uint8_t*>(usage.data()));
		//coap_show_pdu(LOG_WARNING, response);
	});

	coap_register_handler(resourceMem, COAP_REQUEST_GET,
			[](auto, auto, const coap_pdu_t* request, auto,
				coap_pdu_t* response) {
		coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
		memUsageMutex.lock();
		auto usage = std::to_string(memUsage);
		memUsageMutex.unlock();
		coap_add_data(response, usage.size(), reinterpret_cast<const uint8_t*>(usage.data()));
	});

	coap_add_resource(ctx, resourceCpu);
	coap_add_resource(ctx, resourceMem);

	auto cpuCheckThread = std::thread([](){
		while(true) {
			refreshCpuUsage();
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(1s);
		}
	});
	cpuCheckThread.detach();
	
	auto memCheckThread = std::thread([]() {
		while(true) {
			refreshMemUsage();
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(1s);
		}
	});

	std::cerr << "Coap server is a-go\n";
	while(true) {
		coap_io_process(ctx, COAP_IO_WAIT);
	}

	die(ctx, EXIT_SUCCESS);
}
