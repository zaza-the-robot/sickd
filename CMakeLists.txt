cmake_minimum_required(VERSION 2.6)
project(sickd)

find_package(PkgConfig)
pkg_check_modules(PC_LIBSERIAL REQUIRED libserialport)

set(SICK_SRCS
	src/crc_sick.c
	src/events.c
	src/main.c
	src/pls_201.c
)


add_executable(sickd ${SICK_SRCS})
target_link_libraries(sickd ${PC_LIBSERIAL_LIBRARIES})
install(TARGETS sickd RUNTIME DESTINATION bin)
