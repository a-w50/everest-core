// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2021 Pionix GmbH and Contributors to EVerest
const { evlog, boot_module } = require('everestjs');

var price_above_threshold = true;
var price = 0.99;

async function fetch_tibber_api_data(mod) {
  evlog.info('Fetching update from Tibber');

  let schedule = {};
  let entry = {};
  
  if (price_above_threshold) {
    price = 0.99;
    price_above_threshold = false;
  } else {
    price = 0.01;
    price_above_threshold = true;
  }

  var timestamp = new Date();
  timestamp.setTime(timestamp.getTime());
  timestamp.setMinutes(timestamp.getMinutes()-20);

  entry['timestamp'] = new Date(timestamp).toISOString();
  entry['price_per_kwh'] = {
    // Tibber returns the total energy cost including taxes and fees.
    // Add constant offset if needed for other costs.
    value: price,
    currency: "EUR"
  }
  // copy into schedule_import array
  schedule['schedule_import'] = [];
  schedule['schedule_import'].push(entry);

  mod.provides.main.publish.energy_price_schedule(schedule);
};

function start_api_loop(mod) {
  // const update_interval_milliseconds = mod.config.impl.main.update_interval * 60 * 1000;
  const update_interval_milliseconds = 10 * 1000;
  fetch_tibber_api_data(mod);
  setInterval(fetch_tibber_api_data, update_interval_milliseconds, mod);
}

boot_module(async ({ setup, info, config }) => {
  evlog.info('Booting JsTibber!');
}).then((mod) => {
  // Call API for the first time and then set an interval to fetch the data regularly
  start_api_loop(mod);
});
