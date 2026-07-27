cmake_minimum_required(VERSION 3.16)
project(camera_service LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED)

add_library(camera_service_core
    src/camera_intrinsics.cpp
    src/camera_capture.cpp
    src/camera_server.cpp
)
target_include_directories(camera_service_core PUBLIC include)
target_link_libraries(camera_service_core PUBLIC ${OpenCV_LIBS} Eigen3::Eigen)

add_executable(camera_service_daemon src/main.cpp)
target_link_libraries(camera_service_daemon PRIVATE camera_service_core)

add_executable(camera_service_client tools/client_example.cpp)
target_link_libraries(camera_service_client PRIVATE ${OpenCV_LIBS})
