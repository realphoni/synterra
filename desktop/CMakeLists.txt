cmake_minimum_required(VERSION 3.21)
project(synterra_hub VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets Test)
qt_add_executable(synterra-hub main.cpp)
qt_add_resources(synterra-hub hub_resources PREFIX /hub
    BASE ../profile/airootfs/usr/share/icons/hicolor/scalable/apps
    FILES ../profile/airootfs/usr/share/icons/hicolor/scalable/apps/synterra.svg)
target_link_libraries(synterra-hub PRIVATE Qt6::Widgets Qt6::Test)
install(TARGETS synterra-hub RUNTIME DESTINATION bin)
