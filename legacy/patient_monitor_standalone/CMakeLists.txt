cmake_minimum_required(VERSION 3.10)
project(PatientMonitorStandalone VERSION 1.0.0 LANGUAGES CXX)

# 寻找 Qt5 核心库和 Widgets 库
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

# 自动处理 moc、uic、rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 当前目录加入头文件搜索路径
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 添加可执行文件
add_executable(${PROJECT_NAME}
    main.cpp
    mainwindow.cpp
    mainwindow.h
    wavewidget.cpp
    wavewidget.h
)

# 链接 Qt5 库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Widgets
)

# 设置 C++ 标准
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
