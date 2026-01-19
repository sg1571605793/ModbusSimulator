set(INC_DIR ${CMAKE_CURRENT_LIST_DIR})
file(GLOB LIB_MODBUS_SRC ${CMAKE_CURRENT_LIST_DIR}/*.c)

message(STATUS "modbus src: ${LIB_MODBUS_SRC}")
message("------------")

add_library(libmodbus SHARED ${LIB_MODBUS_SRC})
target_include_directories(libmodbus PUBLIC {INC_DIR})

target_link_libraries(libmodbus ws2_32)
