// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 - 2022 Pionix GmbH and Contributors to EVerest

#include "solar_managerImpl.hpp"
#include <chrono>
#include <date/date.h>

namespace module {
namespace main {

void solar_managerImpl::init() {

    set_defaults();

    // Subscribe to powermeter
    mod->r_gridpowermeter->subscribe_powermeter([this](json data) { this->on_grid_powermeter(data); });
    mod->r_chargingdriver->subscribe_session_events([this](json data) { this->on_session_events(data); });

    // External subscriptions
    mod->mqtt.subscribe("/external/solar_manager/set_p_weight", [this](std::string data) { this->on_set_p(atof(data.c_str())); });
    mod->mqtt.subscribe("/external/solar_manager/set_i_weight", [this](std::string data) { this->on_set_i(atof(data.c_str())); });
    mod->mqtt.subscribe("/external/solar_manager/set_d_weight", [this](std::string data) { this->on_set_d(atof(data.c_str())); });
    mod->mqtt.subscribe("/external/solar_manager/set_setpoint", [this](std::string data) { this->on_set_s(atof(data.c_str())); });

    mod->mqtt.subscribe("/external/solar_manager/start", [this](const std::string data) { this->on_start(); });
    mod->mqtt.subscribe("/external/solar_manager/stop", [this](const std::string data) { this->on_stop(); });
    mod->mqtt.subscribe("/external/solar_manager/reset", [this](const std::string data) { this->on_reset(); });
}

void solar_managerImpl::ready() {
    interval_start([this](){this->run_solar_manager();}, config.pid_output_interval);
}

void solar_managerImpl::interval_start(const std::function<void(void)>& func, unsigned int interval_ms) {
    std::thread([func, interval_ms]() { 
        while (true) { 
            auto next_interval_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(interval_ms);
            func();
            std::this_thread::sleep_until(next_interval_time);
        }
    }).detach();
}

void solar_managerImpl::set_defaults() {
    _pid_controller = {
        {"p_term", 0.0},
        {"i_term", 0.0},
        {"d_term", 0.0},
        {"error", 0.0},
        {"p_weight", double(config.pid_p_weight)},
        {"i_weight", double(config.pid_i_weight)},
        {"d_weight", double(config.pid_d_weight)},
        {"setpoint", double(config.pid_setpoint)},
        {"i_limit", double(config.pid_i_limit)},
        {"min_out", double(config.pid_min_output)},
        {"max_out", double(config.pid_max_output)}
    };

    _is_active = false;
    _charging_power = 0.0;
    _charging_state = _chargingState_t(UNINITIALIZED);

    _pause_msg = {"pause_log", "pause"};

    _power_msg = {"power_log", 0.0};
}

void solar_managerImpl::run_solar_manager() {
    set_charging_power();
}

void solar_managerImpl::activate_solar_manager() {
    _is_active = true;
    mod->mqtt.publish("/external/solar_manager/solar_manager_is_active", _is_active);
    publish_solar_manager_is_active(_is_active);
}

void solar_managerImpl::deactivate_solar_manager() {
    _is_active = false;
    mod->mqtt.publish("/external/solar_manager/solar_manager_is_active", _is_active);
    publish_solar_manager_is_active(_is_active);
}

void solar_managerImpl::on_session_events(json val) {
    // Decide between charging / not charging
    if (val.contains("event")) {
        if (val.at("event") == CHARGING_STARTED || val.at("event") == CHARGING_RESUMED) {
            _charging_state = CHARGING_STARTED;
        } else {
            _charging_state = CHARGING_FINISHED;
        }
    }
}

void solar_managerImpl::on_grid_powermeter(json pm) {
    mod->mqtt.publish("/external/solar_manager/solar_manager_is_active", _is_active);

    if (!_is_active) {
        return;
    }

    if (!pm.contains("power_W") || !pm.at("power_W").contains("total")) {  // error: incoming powermeter object incomplete
        return;
    }
    
    const double input_interval = 1.0;
    double error = double(pm.at("power_W").at("total")) - double(_pid_controller.at("setpoint"));

    // proportional term
    _pid_controller.at("p_term") = error;

    // integral term
    double i_term_old = double(_pid_controller.at("i_term"));
    _pid_controller.at("i_term") = i_term_old + (error * input_interval);

    if (_pid_controller.at("i_limit") > 0) {
        if (double(_pid_controller.at("i_term")) > double(_pid_controller.at("i_limit"))) {
            _pid_controller.at("i_term") = _pid_controller.at("i_limit");
        } else if (double(_pid_controller.at("i_term")) < (-1.0 * double(_pid_controller.at("i_limit")))) {
            _pid_controller.at("i_term") = -1.0 * double(_pid_controller.at("i_limit"));
        }
    }

    if (_charging_power < double(_pid_controller.at("min_out"))) {
        EVLOG(debug) << "PID output is lower than minimum, integral term reset to 0.0";
        _pid_controller.at("i_term") = 0.0;
    }
    else if (_charging_power >= double(_pid_controller.at("max_out"))) {
        EVLOG(debug) << "PID output is higher than maximum, integral term can't be increased";
        if (i_term_old < double(_pid_controller.at("i_term"))) {
            _pid_controller.at("i_term") = i_term_old;
        }
    }

    // derivative term
    _pid_controller.at("d_term") = error - double(_pid_controller.at("error"));
    _pid_controller.at("error") = error;

    // publish to data logging
    json msg = {
        {"pid_p", ( double(_pid_controller.at("p_term")) * double(_pid_controller.at("p_weight")) ) },
        {"pid_i", ( double(_pid_controller.at("i_term")) * double(_pid_controller.at("i_weight")) ) },
        {"pid_d", ( double(_pid_controller.at("d_term")) * double(_pid_controller.at("d_weight")) ) },
        {"pid_error", double(_pid_controller.at("error")) },
        {"p_weight", double(_pid_controller.at("p_weight"))},
        {"i_weight", double(_pid_controller.at("i_weight"))},
        {"d_weight", double(_pid_controller.at("d_weight"))},
        {"setpoint", double(_pid_controller.at("setpoint"))}
    };
    // EVLOG(error) << msg; // TODO(LAD): remove me
    publish_logging(msg);

    // calculate charging power
    _charging_power = double(_pid_controller.at("p_weight")) * double(_pid_controller.at("p_term")) + double(_pid_controller.at("i_weight")) * double(_pid_controller.at("i_term")) + double(_pid_controller.at("d_weight")) * double(_pid_controller.at("d_term"));
    if (_charging_power > double(_pid_controller.at("max_out")) ) {
        _charging_power = double(_pid_controller.at("max_out"));
    }
    else if (_charging_power < double(_pid_controller.at("min_out")) ) {
        _charging_power = 0.0;
    }
}

void solar_managerImpl::reset_pid_controller() {
    _pid_controller.at("p_term") = 0.0;
    _pid_controller.at("i_term") = 0.0;
    _pid_controller.at("d_term") = 0.0;
    _pid_controller.at("error") = 0.0;
}

void solar_managerImpl::on_set_p(double val) {
    EVLOG(info) << "Set p_weight to " << val;
    _pid_controller.at("p_weight") = val;
    reset_pid_controller();
}

void solar_managerImpl::on_set_i(double val) {
    EVLOG(info) << "Set i_weight to " << val;
    _pid_controller.at("i_weight") = val;
    reset_pid_controller();
}

void solar_managerImpl::on_set_d(double val) {
    EVLOG(info) << "Set d_weight to " << val;
    _pid_controller.at("d_weight") = val;
    reset_pid_controller();
}

void solar_managerImpl::on_set_s(double val) {
    EVLOG(info) << "Set setpoint to " << val;
    _pid_controller.at("setpoint") = val;
    reset_pid_controller();
}

void solar_managerImpl::on_start() {
    if (_is_active) {
        EVLOG(info) << "PID-Controller is already active..";
        return;
    }
    reset_pid_controller();
    activate_solar_manager();
    EVLOG(debug) << "PID-Controller is starting..";
}

void solar_managerImpl::on_stop() {
    if (!_is_active) {
        EVLOG(info) << "PID-Controller is not active..";
        return;
    }
    deactivate_solar_manager();
    _charging_power = 0.0;
    set_charging_power();
    EVLOG(debug) << "PID-Controller is deactivating..";
}

void solar_managerImpl::on_reset() {
    reset_pid_controller();
}

void solar_managerImpl::set_charging_power() {
    if (!_is_active) {
        return;
    }
    if (_charging_power > 0.0) {
        if (_charging_state != CHARGING_STARTED) {
            mod->r_chargingdriver->call_resume_charging();
            _pause_msg.at("pause_log") = "resume";
            publish_logging(_pause_msg);
        } else {
            json msg = {
                "max_current", double(_charging_power / 230)
            };
            mod->r_chargingdriver->call_set_local_max_current(msg);
            _power_msg.at("power_log") = _charging_power;
            publish_logging(_power_msg);
        }
    } else {
        if (_charging_state == CHARGING_STARTED) {
            mod->r_chargingdriver->call_pause_charging();
            _pause_msg.at("pause_log") = "pause";
            publish_logging(_pause_msg);
        }
    }
}

} // namespace main
} // namespace module
