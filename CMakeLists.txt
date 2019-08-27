cmake_minimum_required(VERSION 2.4.7)
PROJECT(microstepper CXX C)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")

include(GNUInstallDirs)

set (MICROSTEPPER_VERSION_MAJOR 1)
set (MICROSTEPPER_VERSION_MINOR 0)

find_package(INDI REQUIRED)
find_package(ZLIB REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_microstepper.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_microstepper.xml )

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})

include(CMakeCommon)

############# MICROFOCUSER ###############
set(microstepper_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/microstepper.cpp
)

add_executable(indi_microstepper ${microstepper_SRCS})

target_link_libraries(indi_microstepper ${INDI_LIBRARIES} m ${ZLIB_LIBRARY})

install(TARGETS indi_microstepper RUNTIME DESTINATION bin)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_microstepper.xml DESTINATION ${INDI_DATA_DIR})
