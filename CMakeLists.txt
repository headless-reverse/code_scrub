cmake_minimum_required(VERSION 3.16)
project(code_scrub VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(ZLIB REQUIRED)
find_package(LibArchive REQUIRED)

find_path(TREE_SITTER_INCLUDE_DIR NAMES tree_sitter/api.h)
find_library(TREE_SITTER_LIBRARY NAMES tree-sitter)

if(NOT TREE_SITTER_INCLUDE_DIR OR NOT TREE_SITTER_LIBRARY)
    message(WARNING "Tree-sitter libraries not found automatically. Please ensure libtree-sitter is installed.")
endif()

set(SOURCES
    main.cpp
    mainwindow.cpp
    check.cpp
    check_view.cpp
    shrink.cpp
    shrink_view.cpp
    shrink_parser.cpp
    formatter.cpp
    analysis_engine.cpp
)

set(HEADERS
    mainwindow.h
    check.h
    check_view.h
    shrink.h
    shrink_view.h
    shrink_parser.h
    formatter.h
    analysis_engine.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
        ZLIB::ZLIB
        ${LibArchive_LIBRARIES}
        ${TREE_SITTER_LIBRARY}
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LibArchive_INCLUDE_DIRS}
    ${TREE_SITTER_INCLUDE_DIR}
)
