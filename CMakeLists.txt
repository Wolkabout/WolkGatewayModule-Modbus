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

# Update the link and include directories
link_directories(${CMAKE_PREFIX_PATH}/lib)
link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
include_directories(SYSTEM ${CMAKE_PREFIX_PATH}/include)

# Introduce submodules if necessary
option(BUILD_GTEST "" OFF)
option(BUILD_TESTS "" OFF)
option(BUILD_EXAMPLES "" OFF)
option(BUILD_POCO_HTTP_DOWNLOADER "" OFF)
if (NOT TARGET WolkAboutConnector)
    add_subdirectory(WolkConnect-Cpp)
endif ()
if (NOT TARGET MoreModbus)
    add_subdirectory(MoreModbus)
endif ()

# Define the version of the software
set(WOLKGATEWAY_MODBUS_VERSION_MAJOR 1)
set(WOLKGATEWAY_MODBUS_VERSION_MINOR 0)
set(WOLKGATEWAY_MODBUS_VERSION_PATCH 1)
set(WOLKGATEWAY_MODBUS_VERSION_SUFFIX)

set(WOLKGATEWAY_MODBUS_VERSION_STRING "${WOLKGATEWAY_MODBUS_VERSION_MAJOR}.${WOLKGATEWAY_MODBUS_VERSION_MINOR}.${WOLKGATEWAY_MODBUS_VERSION_PATCH}")
if (NOT "${WOLKGATEWAY_MODBUS_VERSION_SUFFIX}" STREQUAL "")
    set(WOLKGATEWAY_MODBUS_VERSION_STRING "${WOLKGATEWAY_MODBUS_VERSION_STRING}-${WOLKGATEWAY_MODBUS_VERSION_SUFFIX}")
endif ()

# WolkAbout Modbus Module
set(MODBUS_SOURCE_FILES modbus/model/DeviceInformation.cpp
        modbus/model/DevicesConfiguration.cpp
        modbus/model/DeviceTemplate.cpp
        modbus/model/MappingType.cpp
        modbus/model/ModuleConfiguration.cpp
        modbus/model/ModuleMapping.cpp
        modbus/model/SerialRtuConfiguration.cpp
        modbus/model/TcpIpConfiguration.cpp
        modbus/module/persistence/JsonFilePersistence.cpp
        modbus/module/ModbusBridge.cpp
        modbus/module/RegisterMappingFactory.cpp
        modbus/module/WolkaboutTemplateFactory.cpp)
set(MODBUS_HEADER_FILES modbus/model/DeviceInformation.h
        modbus/model/DevicesConfiguration.h
        modbus/model/DeviceTemplate.h
        modbus/model/MappingType.h
        modbus/model/ModuleConfiguration.h
        modbus/model/ModuleMapping.h
        modbus/model/SerialRtuConfiguration.h
        modbus/model/TcpIpConfiguration.h
        modbus/module/persistence/JsonFilePersistence.h
        modbus/module/persistence/KeyValuePersistence.h
        modbus/module/ModbusBridge.h
        modbus/module/RegisterMappingFactory.h
        modbus/module/WolkaboutTemplateFactory.h
        modbus/utilities/JsonReaderParser.h)

add_library(${PROJECT_NAME} SHARED ${MODBUS_SOURCE_FILES} ${MODBUS_HEADER_FILES})
target_link_libraries(${PROJECT_NAME} WolkAboutConnector MoreModbus)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_LIBRARY_INCLUDE_DIRECTORY} ${CMAKE_PREFIX_PATH}/include)
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${WOLKGATEWAY_MODBUS_VERSION_STRING} SOVERSION ${WOLKGATEWAY_MODBUS_VERSION_MAJOR}.${WOLKGATEWAY_MODBUS_VERSION_MINOR}.${WOLKGATEWAY_MODBUS_VERSION_PATCH})

# Example
set(APPLICATION_SOURCE_FILES application/Application.cpp)

add_executable(ModbusModule ${APPLICATION_SOURCE_FILES})
target_link_libraries(ModbusModule ${PROJECT_NAME})
target_include_directories(ModbusModule PRIVATE ${PROJECT_SOURCE_DIR})
set_target_properties(ModbusModule PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")

# Create the install rule
include(GNUInstallDirs)
install(DIRECTORY ${CMAKE_LIBRARY_INCLUDE_DIRECTORY} DESTINATION ${CMAKE_INSTALL_PREFIX} PATTERN *.h)
install(DIRECTORY ${CMAKE_PREFIX_PATH}/include DESTINATION ${CMAKE_INSTALL_PREFIX} PATTERN *.h)
install(DIRECTORY ${CMAKE_PREFIX_PATH}/lib/ DESTINATION ${CMAKE_INSTALL_PREFIX}/lib FILES_MATCHING PATTERN "*so*")
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(TARGETS ModbusModule DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(FILES out/moduleConfiguration.json out/devicesConfiguration.json DESTINATION /etc/modbusModule
        PERMISSIONS OWNER_WRITE OWNER_READ GROUP_WRITE GROUP_READ WORLD_READ)
