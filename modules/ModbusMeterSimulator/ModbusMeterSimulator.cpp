// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 - 2022 Pionix GmbH and Contributors to EVerest
#include "ModbusMeterSimulator.hpp"

namespace module {

void ModbusMeterSimulator::init() {
    invoke_init(*p_main);
}

void ModbusMeterSimulator::ready() {
    invoke_ready(*p_main);
}

} // namespace module
