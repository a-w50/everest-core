// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2021 Pionix GmbH and Contributors to EVerest
const { evlog, boot_module } = require('everestjs');

function run_generator(mod) {

  // code
  let time_now = new Date().toISOString();
  let energy_generation_msg = { 
    node_type: "Grid",
    uuid: mod.info.id,
    schedule_export: [
      {
        timestamp: time_now,
        request_parameters: {
          limit_type: "Soft",
          ac_current_A: {
            max_current_A: 77.0,
            min_current_A: 0.0,
            max_phase_count: 3,
            min_phase_count: 1,
            supports_changing_phases_during_charging: false
          }
        },
        price_per_kwh: {
          value: 0.0,
          currency: "EUR",
        }
      }
    ]
  };
  mod.provides.main.publish.energy(energy_generation_msg);

}

function start_api_loop(mod) {
  const update_interval_milliseconds = mod.config.impl.main.update_interval;

  setInterval(run_generator, update_interval_milliseconds, mod);
}

boot_module(async ({ setup, info, config }) => {
  evlog.info('Booting JsDieselGenerator!');
}).then((mod) => {
  // loop
  start_api_loop(mod);
});
