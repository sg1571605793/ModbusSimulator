cmake_minimum_required(VERSION 3.19)
project(ModbusSimulator LANGUAGES CXX C)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt6 REQUIRED COMPONENTS SerialPort)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

add_subdirectory(libmodbus)

qt_add_executable(ModbusSimulator
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    modbusmaster.h modbusmaster.cpp
    modbusslave.h modbusslave.cpp
    modbusregister.h modbusregister.cpp
)

target_link_libraries(ModbusSimulator
    PRIVATE
        Qt::Core
        Qt::Widgets
        libmodbus
)

target_link_libraries(ModbusSimulator PRIVATE Qt6::SerialPort)
target_link_libraries(ModbusSimulator PRIVATE Qt6::Core Qt6::Widgets)

target_include_directories(ModbusSimulator PRIVATE libmodbus)

include(GNUInstallDirs)

install(TARGETS ModbusSimulator
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET ModbusSimulator
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
