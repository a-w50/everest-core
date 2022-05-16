cmake_minimum_required(VERSION 3.10)

# set the project name
project(yeti_fwupdate VERSION 0.1)
# specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)


configure_file(config.h.in config.h)

# add the executable
add_executable(yeti_fwupdate main.cpp ../yeti_comms/evSerial.cpp ../yeti_comms/protobuf/hi2lo.pb.c ../yeti_comms/protobuf/lo2hi.pb.c ../yeti_comms/nanopb/pb_decode.c ../yeti_comms/nanopb/pb_encode.c ../yeti_comms/nanopb/pb_common.c)

target_include_directories(yeti_fwupdate PUBLIC "${PROJECT_BINARY_DIR}" PUBLIC "../yeti_comms/nanopb" PUBLIC "../yeti_comms/protobuf" PUBLIC "../common" PUBLIC "../yeti_comms")
target_link_libraries(yeti_fwupdate PRIVATE Pal::Sigslot Threads::Threads date::date date::date-tz)

install(TARGETS yeti_fwupdate
        DESTINATION ${EVEREST_MOD_YETIDRIVER_DESTINATION})


