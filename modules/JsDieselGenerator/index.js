// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2021 Pionix GmbH and Contributors to EVerest
const { evlog, boot_module } = require('everestjs');

function run_generator(mod) {

  // code
  let energy_generation_msg = { energy_output_W: 1000 };
  mod.provides.main.publish.energy_generation(energy_generation_msg);

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
