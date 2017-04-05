cmake_minimum_required(VERSION 2.6)
project(sickd)

find_package(PkgConfig)
pkg_check_modules(PC_LIBSERIAL REQUIRED libserialport)

add_executable(sickd src/main.c)
target_link_libraries(sickd ${PC_LIBSERIAL_LIBRARIES})
install(TARGETS sickd RUNTIME DESTINATION bin)
