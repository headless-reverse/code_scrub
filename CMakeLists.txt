cmake_minimum_required(VERSION 3.16)
project(code_scrub VERSION 2.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(ZLIB REQUIRED)
find_package(LibArchive REQUIRED)

find_path(TREE_SITTER_INCLUDE_DIR NAMES tree_sitter/api.h)
find_library(TREE_SITTER_LIBRARY NAMES tree-sitter)

find_path(CLANG_INCLUDE_DIR NAMES clang-c/Index.h 
    PATHS 
        /usr/lib/llvm-19/include 
        /usr/lib/llvm-21/include
        /usr/include
        /usr/local/include
)

find_library(CLANG_LIBRARY NAMES clang-19 clang-21 clang libclang 
    PATHS 
        /usr/lib/llvm-19/lib
        /usr/lib/llvm-21/lib
        /usr/lib/x86_64-linux-gnu
)

set(SOURCES
    main.cpp
    mainwindow.cpp
    check.cpp
    check_view.cpp
    shrink.cpp
    shrink_cpp.cpp
    shrink_factory.cpp
    shrink_java.cpp
    shrink_python.cpp
    shrink_web.cpp
    shrink_view.cpp
    shrink_parser.cpp
    formatter.cpp
    analysis_engine.cpp
    language.cpp
    ast_highlighter.cpp
    archive_worker.cpp
    clang_analyzer.cpp
)

set(HEADERS
    mainwindow.h
    check.h
    check_view.h
    shrink.h
    shrink_base.h
    shrink_cpp.h
    shrink_factory.h
    shrink_java.h
    shrink_python.h
    shrink_web.h
    shrink_view.h
    shrink_parser.h
    formatter.h
    analysis_engine.h
    language.h
    ast_highlighter.h
    archive_worker.h
    clang_analyzer.h
    language_defs.h
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

if(CLANG_LIBRARY AND CLANG_INCLUDE_DIR)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CLANG_LIBRARY})
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_LIBCLANG)
    message(STATUS "Wykryto libclang: ${CLANG_LIBRARY}")
    message(STATUS "Katalog naglowkow libclang: ${CLANG_INCLUDE_DIR}")
else()
    message(WARNING "Nie odnaleziono pelnego pakietu libclang. Narzedzie Clang Analyzer bedzie dzialac w trybie fallback.")
endif()

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LibArchive_INCLUDE_DIRS}
    ${TREE_SITTER_INCLUDE_DIR}
    ${CLANG_INCLUDE_DIR}
)
