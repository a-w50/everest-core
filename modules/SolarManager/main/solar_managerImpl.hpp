// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#ifndef MAIN_SOLAR_MANAGER_IMPL_HPP
#define MAIN_SOLAR_MANAGER_IMPL_HPP

//
// AUTO GENERATED - MARKED REGIONS WILL BE KEPT
// template version 1
//

#include <generated/solar_manager/Implementation.hpp>

#include "../SolarManager.hpp"

// ev@75ac1216-19eb-4182-a85c-820f1fc2c091:v1
// insert your custom include headers here
// ev@75ac1216-19eb-4182-a85c-820f1fc2c091:v1

namespace module {
namespace main {

struct Conf {
    double pid_setpoint;
    double pid_p_weight;
    double pid_i_weight;
    double pid_d_weight;
    int pid_output_interval;
    double pid_i_limit;
    double pid_min_output;
    double pid_max_output;
};

class solar_managerImpl : public solar_managerImplBase {
public:
    solar_managerImpl() = delete;
    solar_managerImpl(Everest::ModuleAdapter* ev, const Everest::PtrContainer<SolarManager>& mod, Conf& config) :
        solar_managerImplBase(ev, "main"), mod(mod), config(config){};

    // ev@8ea32d28-373f-4c90-ae5e-b4fcc74e2a61:v1
    // insert your public definitions here
    // ev@8ea32d28-373f-4c90-ae5e-b4fcc74e2a61:v1

protected:
    // no commands defined for this interface

    // ev@d2d1847a-7b88-41dd-ad07-92785f06f5c4:v1
    // insert your protected definitions here
    // ev@d2d1847a-7b88-41dd-ad07-92785f06f5c4:v1

private:
    const Everest::PtrContainer<SolarManager>& mod;
    const Conf& config;

    virtual void init() override;
    virtual void ready() override;

    // ev@3370e4dd-95f4-47a9-aaec-ea76f34a66c9:v1
    // insert your private definitions here
    enum _chargingState_t {
        ACTIVE,
        DEACTIVATED,
        SESSION_STARTED,
        CHARGING_STARTED,
        CHARGING_PAUSED_EV,
        CHARGING_PAUSED_EVSE,
        CHARGING_RESUMED,
        CHARGING_FINISHED,
        SESSION_FINISHED,
        ERROR,
        PERMANENT_FAULT,
        UNINITIALIZED
    };

    json _pid_controller;
    bool _is_active;
    double _charging_power;
    _chargingState_t _charging_state;
    json _pause_msg;
    json _power_msg;

    void interval_start(const std::function<void(void)>& func, unsigned int interval_ms);
    void set_defaults();
    void run_solar_manager();
    void activate_solar_manager();
    void deactivate_solar_manager();
    void on_session_events(json val);
    void on_grid_powermeter(json pm);
    void reset_pid_controller();
    void on_set_p(double val);
    void on_set_i(double val);
    void on_set_d(double val);
    void on_set_s(double val);
    void on_start();
    void on_stop();
    void on_reset();
    void set_charging_power();
    // ev@3370e4dd-95f4-47a9-aaec-ea76f34a66c9:v1
};

// ev@3d7da0ad-02c2-493d-9920-0bbbd56b9876:v1
// insert other definitions here
// ev@3d7da0ad-02c2-493d-9920-0bbbd56b9876:v1

} // namespace main
} // namespace module

#endif // MAIN_SOLAR_MANAGER_IMPL_HPP
