cmake_minimum_required(VERSION 3.21)
project(synaqua VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets Quick Qml DBus Test)
find_package(KF6Service REQUIRED)
find_package(KF6KIO REQUIRED)
find_package(KF6GlobalAccel REQUIRED)
find_package(KF6WindowSystem REQUIRED)
find_package(LayerShellQt REQUIRED)
qt_add_executable(synaqua-shell main.cpp shell.cpp shell.h)
qt_add_resources(synaqua-shell shell_resources PREFIX /synaqua FILES
    qml/Main.qml qml/DockButton.qml qml/GlassPane.qml
    ../profile/airootfs/usr/share/icons/hicolor/scalable/apps/synterra.svg)
target_link_libraries(synaqua-shell PRIVATE Qt6::Widgets Qt6::Quick Qt6::Qml Qt6::DBus Qt6::Test
    KF6::Service KF6::KIOGui KF6::GlobalAccel KF6::WindowSystem LayerShellQt::Interface)
target_compile_definitions(synaqua-shell PRIVATE SYNAQUA_VERSION="${PROJECT_VERSION}")
install(TARGETS synaqua-shell RUNTIME DESTINATION bin)
