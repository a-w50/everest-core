##
## SPDX-License-Identifier: Apache-2.0
## Copyright 2020 - 2021 Pionix GmbH and Contributors to EVerest
##
LD_LIBRARY_PATH=`pwd`/_deps/everest-framework-build/lib dist/bin/manager --log_conf ../config/logging.ini --main_dir dist --schemas_dir dist/schemas --conf ../config/config-sil_solar_manager.json --modules_dir dist/modules --interfaces_dir dist/interfaces $@
