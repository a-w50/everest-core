// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2021 Pionix GmbH and Contributors to EVerest
const { evlog, boot_module } = require('everestjs');

let _output_power_W = 6000; 

function run_generator(mod) {

  // code
  let time_now = new Date().toISOString();
  let energy_generation_msg = { 
    node_type: "LocalSource",
    uuid: mod.info.id,
    schedule_export: [
      {
        timestamp: time_now,
        request_parameters: {
          limit_type: "Soft",
          ac_current_A: {
            max_current_A: ( _output_power_W / 230 ),
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

function setOutputPower(mod, inputData) {

  let power_to_be_set = _output_power_W;

  if (typeof inputData.limits_export != "undefined") {
    if (typeof inputData.limits_export.request_parameters != "undefined") {
      if (typeof inputData.limits_export.request_parameters.ac_current_A != "undefined") {
        if (typeof inputData.limits_export.request_parameters.ac_current_A.current_A != "undefined") {
          power_to_be_set = inputData.limits_export.request_parameters.ac_current_A.current_A * 230;
          evlog.info("setOutputPower to: ", power_to_be_set);
        }
      }
    }
  }

  if (0 <= power_to_be_set){
    if (power_to_be_set <= (32 * 230) ){
      _output_power_W = power_to_be_set;
    } else {
      _output_power_W = (32 * 230);
    }
  } else {
    _output_power_W = 0;
  }
}

function start_api_loop(mod) {
  const update_interval_milliseconds = mod.config.impl.main.update_interval;

  setInterval(run_generator, update_interval_milliseconds, mod);
}

boot_module(async ({ setup, info, config }) => {
  evlog.info('Booting JsDieselGenerator!');

  setup.provides.main.register.enforce_limits(setOutputPower);

}).then((mod) => {
  // loop
  start_api_loop(mod);
});
