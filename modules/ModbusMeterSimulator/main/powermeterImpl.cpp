// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2021 Pionix GmbH and Contributors to EVerest
#include "powermeterImpl.hpp"

#include <chrono>
#include <random>

using namespace everest;

namespace module {
namespace main {

void powermeterImpl::init() {
    this->init_modbus_client();
}

void powermeterImpl::ready() {
    this->meter_loop_thread = std::thread( [this] { run_meter_loop(); } );
}

void powermeterImpl::run_meter_loop() {
    EVLOG(debug) << "Starting ModbusMeterSimulator loop";
    int32_t power_in, power_out, energy_in, energy_out;
    while (true) {

        // {"energy_Wh_export":{"total":51771719},"energy_Wh_import":{"total":14291560},"meter_id":"MODBUS_POWERMETER","power_W":{"total":-10469},"timestamp":1651230038662}
        srand(time(0));
        power_in = (uint32_t) (0);     // 0 on measurement day @Marco
        power_out = (uint32_t) (8800) + ( rand() % 3000 );  // normal day, noon, ~8000..~12000 @Marco
        energy_in = (uint32_t) 14291560; // 14291560
        energy_out = (uint32_t) 51773569; // 51773569

        uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        // Publishing relevant vars
        json j;
        j["timestamp"] = timestamp;
        j["meter_id"] = "MODBUS_POWERMETER";
        j["energy_Wh_import"]["total"] = energy_in;
        j["energy_Wh_export"]["total"] = energy_out;

        j["power_W"]["total"] = power_in - power_out;

        publish_powermeter(j);

        std::this_thread::sleep_for(std::chrono::milliseconds(config.update_interval));
    }
}

void powermeterImpl::init_modbus_client() {
    EVLOG(critical) << "Initializing modbus client simulation...";
}

std::string powermeterImpl::handle_get_signed_meter_value(std::string& auth_token) {
    return "NOT_AVAILABLE";
}

} // namespace main
} // namespace module
