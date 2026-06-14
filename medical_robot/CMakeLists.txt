cmake_minimum_required(VERSION 3.10)
project(MedicalRobot VERSION 1.0.0 LANGUAGES CXX)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Network Multimedia)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SOURCES
    main.cpp
    ui/mainwindow.cpp
    ui/smarttriage.cpp
    ui/patientmonitor.cpp
    core/offline_triage.cpp
    core/config.cpp
    core/triageengine.cpp
    network/apiclient.cpp
    network/voicemanager.cpp
)

set(HEADERS
    ui/mainwindow.h
    ui/smarttriage.h
    ui/patientmonitor.h
    core/offline_triage.h
    core/config.h
    core/triageengine.h
    core/datamodel.h
    network/apiclient.h
    network/voicemanager.h
)

add_executable(MedicalRobot ${SOURCES} ${HEADERS})

target_link_libraries(MedicalRobot PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
    Qt5::Multimedia
)

target_include_directories(MedicalRobot PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/network
)

set_target_properties(MedicalRobot PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
