#!/bin/sh

##
## SPDX-License-Identifier: Apache-2.0
## Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
##
LD_LIBRARY_PATH=`pwd`/build/_deps/everest-framework-build/lib \
build/dist/bin/manager \
--log_conf config/logging.ini \
--main_dir build/dist \
--schemas_dir build/dist/schemas \
--conf config/config-sil-ocpp.json \
--modules_dir build/dist/modules \
--interfaces_dir build/dist/interfaces
