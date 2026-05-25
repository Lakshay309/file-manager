cmake_minimum_required(VERSION 3.20)

project(File_Manager)

set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

add_executable(${PROJECT_NAME} 
        src/main.cpp
        src/window/MainWindow.cpp        
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets)
