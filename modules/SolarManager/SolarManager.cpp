// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 - 2022 Pionix GmbH and Contributors to EVerest
#include "SolarManager.hpp"

namespace module {

void SolarManager::init() {
    invoke_init(*p_main);
}

void SolarManager::ready() {
    invoke_ready(*p_main);
}

} // namespace module
