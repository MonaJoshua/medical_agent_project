cmake_minimum_required(VERSION 3.10)
project(SmartTriageStandalone VERSION 1.0.0 LANGUAGES CXX)

# 寻找 Qt5 核心库、Widgets 库和 Network 库
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Network)

# 自动处理 moc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 添加可执行文件
add_executable(${PROJECT_NAME}
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

# 链接 Qt5 库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
)

# 设置 C++ 标准
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
