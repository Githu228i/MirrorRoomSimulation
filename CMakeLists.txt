cmake_minimum_required(VERSION 3.16)

project(MirrorRoomSimulation VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    mirror.h mirror.cpp
            points.h points.cpp
            frontendoperators.h frontendoperators.cpp
            backendoperators.h backendoperators.cpp
            polygon.h polygon.cpp
)

set(TS_FILES
    MirrorRoomSimulation_ru_RU.ts
)

qt_add_executable(MirrorRoomSimulation
    ${PROJECT_SOURCES}
)

qt_add_translations(MirrorRoomSimulation
    TS_FILES ${TS_FILES}
)

target_link_libraries(MirrorRoomSimulation PRIVATE
    Qt6::Widgets
)

set_target_properties(MirrorRoomSimulation PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)

install(TARGETS MirrorRoomSimulation
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(MirrorRoomSimulation)
