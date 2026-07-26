cmake_minimum_required(VERSION 3.10)
project(MedicalRobot VERSION 1.0.0 LANGUAGES CXX)

# 查找 Qt5 必需模块
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Network Multimedia)

# 自动处理 moc/uic/rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)          # 自动编译 .qrc 资源文件
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 源文件列表（按目录组织）
set(SOURCES
    main.cpp
    ui/mainwindow.cpp
    ui/smarttriage.cpp
    ui/patientmonitor.cpp
    ui/voicechatwidget.cpp
    ui/wavewidget.cpp
    ui/moretoolswidget.cpp  
    ui/registrationrecordswidget.cpp
    core/offline_triage.cpp
    core/config.cpp
    core/triageengine.cpp
    network/apiclient.cpp
    network/voicemanager.cpp
    thememanager.cpp
    resources.qrc
)

# 头文件列表（对应源文件）
set(HEADERS
    ui/mainwindow.h
    ui/smarttriage.h
    ui/patientmonitor.h
    ui/voicechatwidget.h
    ui/wavewidget.h
    ui/moretoolswidget.h  
    ui/registrationrecordswidget.h
    core/offline_triage.h
    core/config.h
    core/triageengine.h
    core/datamodel.h
    network/apiclient.h
    network/voicemanager.h
    thememanager.h
)

# 可执行文件
add_executable(MedicalRobot ${SOURCES} ${HEADERS})

# 链接 Qt5 库
target_link_libraries(MedicalRobot PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
    Qt5::Multimedia
)

# 包含目录（方便 #include 直接引用）
target_include_directories(MedicalRobot PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/network
)

# 编译选项（可选）
target_compile_definitions(MedicalRobot PRIVATE
    QT_DEPRECATED_WARNINGS
    QT_DISABLE_DEPRECATED_BEFORE=0x060000
)

# C++ 标准
set_target_properties(MedicalRobot PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)