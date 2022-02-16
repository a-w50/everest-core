// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2021 Pionix GmbH and Contributors to EVerest
#include "yeti_simulation_controlImpl.hpp"

namespace module {
namespace yeti_simulation_control {

Everest::json simulation_feedback_to_json(const SimulationFeedback& s) {
    Everest::json j;

    j["pwm_duty_cycle"] = s.pwmDutyCycle;
    j["relais_on"] = s.relais_on;
    j["evse_pwm_running"] = s.evse_pwm_running;
    j["evse_pwm_voltage_hi"] = s.evse_pwm_voltage_hi;
    j["evse_pwm_voltage_lo"] = s.evse_pwm_voltage_lo;

    return j;
}

SimulationData json_to_simulation_data(Object& v) {
    SimulationData s;

    s.cp_voltage = v["cp_voltage"];
    s.diode_fail = v["diode_fail"];
    s.error_e = v["error_e"];
    s.pp_resistor = v["pp_resistor"];
    s.rcd_current = v["rcd_current"];

    s.currentL1 = v["currents"]["L1"];
    s.currentL2 = v["currents"]["L2"];
    s.currentL3 = v["currents"]["L3"];
    s.currentN = v["currents"]["N"];

    s.voltageL1 = v["voltages"]["L1"];
    s.voltageL2 = v["voltages"]["L2"];
    s.voltageL3 = v["voltages"]["L3"];

    s.freqL1 = v["frequencies"]["L1"];
    s.freqL2 = v["frequencies"]["L2"];
    s.freqL3 = v["frequencies"]["L3"];

    return s;
}

SimulationData value_to_simulation_data(::yeti_simulation_control::Value& v) {
    SimulationData s;

    s.cp_voltage = v.cp_voltage.value();
    s.diode_fail = v.diode_fail.value();
    s.error_e = v.error_e.value();
    s.pp_resistor = v.pp_resistor.value();
    s.rcd_current = v.rcd_current.value();

    s.currentL1 = v.currents.value().L1.value();
    s.currentL2 = v.currents.value().L2.value();
    s.currentL3 = v.currents.value().L3.value();
    s.currentN = v.currents.value().N.value();

    s.voltageL1 = v.voltages.value().L1.value();
    s.voltageL2 = v.voltages.value().L2.value();
    s.voltageL3 = v.voltages.value().L3.value();

    s.freqL1 = v.frequencies.value().L1.value();
    s.freqL2 = v.frequencies.value().L2.value();
    s.freqL3 = v.frequencies.value().L3.value();

    return s;
}

void yeti_simulation_controlImpl::init() {
    mod->serial.signalSimulationFeedback.connect(
        [this](const SimulationFeedback& s) { publish_simulation_feedback(simulation_feedback_to_json(s)); });
}

void yeti_simulation_controlImpl::ready() {
}

void yeti_simulation_controlImpl::handle_enable(bool& value) {
    EVLOG(info) << "void YetiDriverModule::Yeti_simulation_control::enable: " << value;
    mod->serial.enableSimulation(value);
};

void yeti_simulation_controlImpl::handle_setSimulationData(::yeti_simulation_control::Value& value){
    mod->serial.setSimulationData(value_to_simulation_data(value));
};

} // namespace yeti_simulation_control
} // namespace module
