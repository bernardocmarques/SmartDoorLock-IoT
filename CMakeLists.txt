# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# (Not part of the boilerplate)
# This example uses an extra component for common functions such as Wi-Fi and Ethernet connection.
set(EXTRA_COMPONENT_DIRS
        "$ENV{IDF_PATH}/examples/common_components"
        "$ENV{IDF_PATH}/tools/unit-test-app/components/test_utils/"
        )



include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(tcp_server)
add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)
