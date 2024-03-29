# Copyright 2020 WolkAbout Technology s.r.o.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.5)
project(WolkGatewayModbusModule C CXX)

set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS 1)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}     -Wall -Wextra -pedantic                                                    \
                                            -pedantic-errors -Wcast-align                                              \
                                            -Wcast-qual -Wconversion                                                   \
                                            -Wdisabled-optimization                                                    \
                                            -Wfloat-equal -Wformat=2                                                   \
                                            -Werror=init-self -Werror=return-type                                      \
                                            -Werror=missing-field-initializers -Wmissing-format-attribute              \
                                            -Wmissing-include-dirs -Wmissing-noreturn                                  \
                                            -Werror=pointer-arith                                                      \
                                            -Wno-packed  -Wno-padded -Wredundant-decls                                 \
                                            -Werror=shadow -Werror=stack-protector                                     \
                                            -Wstrict-aliasing=2                                                        \
                                            -Werror=unreachable-code -Wno-unused                                       \
                                            -Wvariadic-macros                                                          \
                                            -Wwrite-strings -Werror=non-virtual-dtor")

# Set the variables
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_INCLUDE_DIRECTORY ${CMAKE_BINARY_DIR}/include)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/build)

# Add all the link directories
link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
link_directories(${CMAKE_PREFIX_PATH}/lib)

# WolkAbout c++ SDK and MoreModbus
if (NOT TARGET WolkGatewayModule)
    add_subdirectory(WolkGatewayModule-SDK-Cpp)
endif ()
if (NOT TARGET MoreModbus)
    add_subdirectory(MoreModbus)
endif ()

# WolkAbout Modbus Module
include_directories("src")

file(GLOB_RECURSE HEADER_FILES "src/*.h" "src/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "src/*.c" "src/*.cpp")

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} WolkGatewayModule MoreModbus)
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")

# Example
include_directories("application")
file(GLOB_RECURSE EXAMPLE_SOURCE_FILES "application/*.cpp")

add_executable(modbusModule ${EXAMPLE_SOURCE_FILES})
target_link_libraries(modbusModule ${PROJECT_NAME})
set_target_properties(modbusModule PROPERTIES INSTALL_RPATH "$ORIGIN/lib")

# Place the library files in /usr/lib
install(DIRECTORY ${CMAKE_PREFIX_PATH}/lib DESTINATION ${CMAKE_INSTALL_PREFIX}
        FILES_MATCHING
        PATTERN "*so*")
install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} DESTINATION ${CMAKE_INSTALL_PREFIX}
        FILES_MATCHING
        PATTERN "*so*")

# Place the executable file in /usr/bin
install(TARGETS modbusModule DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

# Place the config files and service unit file.
install(FILES out/moduleConfiguration.json out/devicesConfiguration.json DESTINATION /etc/modbusModule
        PERMISSIONS OWNER_WRITE OWNER_READ GROUP_WRITE GROUP_READ WORLD_READ)

install(CODE "MESSAGE(\"Installed modbusModule. Configuration files for the module are placed in /etc/modbusModule.\")")

add_subdirectory(cmake)

