cmake_minimum_required(VERSION 3.10)
project(app)

# 手动指定编译器（根据实际情况修改路径）
if(WIN32)
    set(CMAKE_C_COMPILER "D:\\mingw64\\bin\\gcc.exe")
    set(CMAKE_CXX_COMPILER "D:\\mingw64\\bin\\g++.exe")
endif()

# 设置输出目录
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin")  # 可执行文件输出到项目根目录的bin文件夹
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 确保Qt5模块被正确找到
find_package(Qt5 COMPONENTS Widgets Core Gui REQUIRED)

# 启用自动处理Qt的MOC、UIC和RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 源文件列表
set(SOURCES
    main.cpp
    main_window.cpp
    network_canvas.cpp
    client_node.cpp
    link.cpp
    monte_carlo_path_finder.cpp
    steiner_path_finder.cpp
    data_processor.cpp
)

# 头文件列表
set(HEADERS
    main_window.h
    network_canvas.h
    client_node.h
    link.h
    monte_carlo_path_finder.h
    steiner_path_finder.h
    path_search_algorithm.h
    data_processor.h
)

# 创建可执行文件
add_executable(app
    ${SOURCES}
    ${HEADERS}
)

# 链接Qt5库
target_link_libraries(app
    Qt5::Widgets
    Qt5::Core
    Qt5::Gui
)
    