// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2021 Pionix GmbH and Contributors to EVerest
#include "EvseManager.hpp"
#include <chrono>
#include <date/date.h>

namespace module {

void EvseManager::init() {
    local_three_phases = config.three_phases;
    invoke_init(*p_evse);
    invoke_init(*p_energy_grid);
    authorization_available = false;
    reserved = false;
    reservation_id = 0;
}

void EvseManager::ready() {
    hw_capabilities = r_bsp->call_get_hw_capabilities();

    // Maybe override with user setting for this EVSE
    if (config.max_current < hw_capabilities.at("max_current_A")) {
        hw_capabilities.at("max_current_A") = config.max_current;
    }

    local_max_current_limit = hw_capabilities.at("max_current_A");

    // Maybe limit to single phase by user setting if possible with HW
    if (!config.three_phases && hw_capabilities.at("min_phase_count") == 1) {
        hw_capabilities.at("max_phase_count") = 1;
        local_three_phases = false;
    } else if (hw_capabilities.at("max_phase_count") == 3) {
        local_three_phases = true; // other configonfigurations currently not supported by HW
    }

    charger = std::unique_ptr<Charger>(new Charger(r_bsp));
    r_bsp->subscribe_event([this](std::string event) { charger->processEvent(event); });

    r_bsp->subscribe_nr_of_phases_available([this](int n) { signalNrOfPhasesAvailable(n); });

    r_powermeter->subscribe_powermeter([this](json p) {
        // Inform charger about current charging current. This is used for slow OC detection.
        charger->setCurrentDrawnByVehicle(p["current_A"]["L1"], p["current_A"]["L2"], p["current_A"]["L3"]);

        // Store local cache
        latest_powermeter_data = p;
    });

    r_auth->subscribe_authorization_available([this](bool a) {
        // Listen to authorize events and cache locally.
        authorization_available = a;
    });

    charger->signalAuthRequired.connect([this]() {
        // The charger indicates it requires auth now. It will retry if we cannot give auth right now.
        if (authorization_available) {
            boost::variant<boost::blank, std::string> auth = r_auth->call_get_authorization();
            if (auth.which() == 1) {
                charger->Authorize(true, boost::get<std::string>(auth));
            }
        }
    });

    // Cancel reservations if charger is disabled or faulted
    charger->signalEvent.connect([this](Charger::EvseEvent s) {
        if (s == Charger::EvseEvent::Disabled || s == Charger::EvseEvent::PermanentFault)
            cancel_reservation();
    });

    invoke_ready(*p_evse);
    invoke_ready(*p_energy_grid);

    charger->setup(local_three_phases, config.has_ventilation, config.country_code, config.rcd_enabled);
    //  start with a limit of 0 amps. We will get a budget from EnergyManager that is locally limited by hw caps.
    charger->setMaxCurrent(0.0F, std::chrono::system_clock::now());
    charger->run();
    charger->enable();
}

json EvseManager::get_latest_powermeter_data() {
    return latest_powermeter_data;
}

json EvseManager::get_hw_capabilities() {
    return hw_capabilities;
}

std::string EvseManager::reserve_now(const int _reservation_id, const std::string& token,
                                     const std::chrono::system_clock::time_point& valid_until,
                                     const std::string& parent_id) {

    // is the evse Unavailable?
    if (charger->getCurrentState() == Charger::EvseState::Disabled)
        return "Unavailable";

    // is the evse faulted?
    if (charger->getCurrentState() == Charger::EvseState::Faulted)
        return "Faulted";

    // is the reservation still valid in time?
    if (std::chrono::system_clock::now() > valid_until)
        return "Rejected";

    // is the connector currently ready to accept a new car?
    if (charger->getCurrentState() != Charger::EvseState::Idle)
        return "Occupied";

    // is it already reserved with a different reservation_id?
    if (reservation_valid() && _reservation_id != reservation_id)
        return "Rejected";

    // accept new reservation
    reserved_auth_token = token;
    reservation_valid_until = valid_until;
    reserved_auth_token_parent_id = parent_id;
    reserved = true;

    // publish event to other modules
    json se;
    se["event"] = "ReservationStart";
    se["reservation_start"]["reservation_id"] = reservation_id;
    se["reservation_start"]["parent_id"] = parent_id;

    signalReservationEvent(se);

    return "Accepted";

    // FIXME TODO:
    /*
        A reservation SHALL be terminated on the Charge Point when either (1) a transaction is started for the reserved
    idTag or parent idTag and on the reserved connector or any connector when the reserved connectorId is 0, or (2)
    when the time specified in expiryDate is reached, or (3) when the Charge Point or connector are set to Faulted or
    Unavailable.

    When a reservation expires, the Charge Point SHALL terminate the reservation and make the connector
    available. The Charge Point SHALL send a status notification to notify the Central System that the reserved
    connector is now available.

    ReservationStart / End events with reason

    reservationEnd with reason Charging Started with it is not implemented yet

    EvseManager: add ID and connector nr

    reservation expiry is not regularly checked to send out event
    */
}

bool EvseManager::updateLocalMaxCurrentLimit(float max_current) {
    if (max_current >= 0.0F && max_current < EVSE_ABSOLUTE_MAX_CURRENT) {
        local_max_current_limit = max_current;
        
        // wait for EnergyManager to assign optimized current on next opimizer run

        return true;
    }
    return false;
}

bool EvseManager::cancel_reservation() {
    if (reservation_valid()) {
        reserved = false;

        // publish event to other modules
        json se;
        se["event"] = "ReservationEnd";
        se["reservation_end"]["reason"] = "Cancelled";
        se["reservation_end"]["reservation_id"] = reservation_id;

        signalReservationEvent(se);

        return true;
    }

    reserved = false;
    return false;
}

float EvseManager::getLocalMaxCurrentLimit() {
    return local_max_current_limit;
}

bool EvseManager::reservation_valid() {
    if (reserved) {
        if (std::chrono::system_clock::now() < reservation_valid_until) {
            // still valid
            return true;
        } else {
            // expired
            // publish event to other modules
            json se;
            se["event"] = "ReservationEnd";
            se["reservation_end"]["reason"] = "Expired";
            se["reservation_end"]["reservation_id"] = reservation_id;

            signalReservationEvent(se);
            reserved = false;
        }
    }
    // no active reservation
    return false;
}

} // namespace module
