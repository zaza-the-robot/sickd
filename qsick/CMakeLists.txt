cmake_minimum_required(VERSION 2.6)
project(qsick`)

find_package(PkgConfig)
pkg_check_modules(PC_LIBQCUSTOMPLOT REQUIRED qcustomplot-qt5)

find_package(Qt5Widgets)
# Run moc automatically when needed
set(CMAKE_AUTOMOC ON)

set(QSICK_SRCS
	qsick.cpp
)

add_executable(qsick ${QSICK_SRCS})
target_link_libraries(qsick Qt5::Widgets ${PC_LIBQCUSTOMPLOT_LIBRARIES} libsickd)
