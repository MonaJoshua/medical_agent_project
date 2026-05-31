cmake_minimum_required(VERSION 3.10)
project(MedicalRobot VERSION 1.0.0 LANGUAGES CXX)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Network)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(MedicalRobot
    main.cpp
    mainwindow.cpp
    mainwindow.h
    patientmonitor.cpp
    patientmonitor.h
    smarttriage.cpp
    smarttriage.h
    offline_triage.cpp
    offline_triage.h
    config.cpp
    config.h
)

target_link_libraries(MedicalRobot PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
)

set_target_properties(MedicalRobot PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
