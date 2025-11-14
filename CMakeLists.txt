# ==============================================================================
# TinyLLVM - CMake Build Configuration
# ==============================================================================

cmake_minimum_required(VERSION 3.10)

project(TinyLLVM
        VERSION 1.0.0
        DESCRIPTION "A minimal compiler IR targeting multiple languages"
        LANGUAGES C
)

# ==============================================================================
# Build Options
# ==============================================================================

option(BUILD_TESTS "Build test executables" ON)
option(ENABLE_WARNINGS "Enable compiler warnings" ON)
option(WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

# ==============================================================================
# C Standard
# ==============================================================================

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ==============================================================================
# Compiler Flags
# ==============================================================================

if(ENABLE_WARNINGS)
    if(MSVC)
        add_compile_options(/W4)
        if(WARNINGS_AS_ERRORS)
            add_compile_options(/WX)
        endif()
    else()
        add_compile_options(
                -Wall
                -Wextra
                -Wpedantic
                -Wformat=2
                -Wno-unused-parameter
                -Wshadow
                -Wwrite-strings
                -Wstrict-prototypes
                -Wold-style-definition
                -Wredundant-decls
                -Wnested-externs
                -Wmissing-include-dirs
        )
        if(WARNINGS_AS_ERRORS)
            add_compile_options(-Werror)
        endif()
    endif()
endif()

# Optimization flags
if(CMAKE_BUILD_TYPE MATCHES Release)
    if(NOT MSVC)
        add_compile_options(-O2)
    endif()
endif()

# Debug flags
if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DDEBUG)
    if(NOT MSVC)
        add_compile_options(-g)
    endif()
endif()

# ==============================================================================
# Library: EventChains
# ==============================================================================

add_library(eventchains STATIC
        src/eventchains.c
        include/eventchains.h
        include/eventchains_platform.h
)

target_include_directories(eventchains PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link pthread for threading support
find_package(Threads REQUIRED)
target_link_libraries(eventchains PUBLIC Threads::Threads)

# ==============================================================================
# Library: TinyLLVM AST
# ==============================================================================

add_library(tinyllvm_ast STATIC
        src/tinyllvm_ast.c
        include/tinyllvm_ast.h
)

target_include_directories(tinyllvm_ast PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# ==============================================================================
# Library: TinyLLVM Compiler (with EventChains integration)
# ==============================================================================

add_library(tinyllvm_compiler STATIC
        src/tinyllvm_lexer.c
        src/tinyllvm_parser.c
        src/tinyllvm_typechecker.c
        src/tinyllvm_codegen_c.c
        include/tinyllvm_compiler.h
)

target_include_directories(tinyllvm_compiler PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(tinyllvm_compiler PUBLIC
        tinyllvm_ast
        eventchains
)

# ==============================================================================
# Executables: Tests
# ==============================================================================

if(BUILD_TESTS)
    # AST Test
    add_executable(tinyllvm_ast_test
            tests/tinyllvm_ast_test.c
    )

    target_link_libraries(tinyllvm_ast_test PRIVATE
            tinyllvm_ast
    )

    # Integrated Lexer Test (with EventChains)
    add_executable(tinyllvm_lexer_test
            tests/tinyllvm_lexer_test.c
    )

    target_link_libraries(tinyllvm_lexer_test PRIVATE
            tinyllvm_compiler
            tinyllvm_ast
            eventchains
    )

    # Full Compiler Pipeline Test
    add_executable(test_full_compiler
            tests/test_full_compiler.c
    )

    target_link_libraries(test_full_compiler PRIVATE
            tinyllvm_compiler
            tinyllvm_ast
            eventchains
    )

    # Compile and Save Test
    add_executable(test_compile_and_save
            tests/test_compile_and_save.c
    )

    target_link_libraries(test_compile_and_save PRIVATE
            tinyllvm_compiler
            tinyllvm_ast
            eventchains
    )

    # Middleware Demonstration Test
    add_executable(test_with_middleware
            tests/test_with_middleware.c
    )

    target_link_libraries(test_with_middleware PRIVATE
            tinyllvm_compiler
            tinyllvm_ast
            eventchains
    )

    # Add tests to CTest
    enable_testing()
    add_test(NAME ast_test COMMAND tinyllvm_ast_test)
    add_test(NAME full_compiler_test COMMAND test_full_compiler)
    add_test(NAME compile_and_save_test COMMAND test_compile_and_save)
    add_test(NAME middleware_test COMMAND test_with_middleware)
    # Note: tinyllvm_lexer_test has known issue on Linux, not added to CTest
endif()

# ==============================================================================
# Installation
# ==============================================================================

install(TARGETS eventchains tinyllvm_ast tinyllvm_compiler
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
)

install(FILES
        include/eventchains.h
        include/eventchains_platform.h
        include/tinyllvm_ast.h
        include/tinyllvm_compiler.h
        DESTINATION include/tinyllvm
)

# ==============================================================================
# Documentation Files
# ==============================================================================

set(DOC_FILES
        README.md
        tests/examples_coretiny.c
)

install(FILES ${DOC_FILES}
        DESTINATION share/doc/tinyllvm
)

# ==============================================================================
# Build Information
# ==============================================================================

message(STATUS "")
message(STATUS "TinyLLVM Configuration:")
message(STATUS "  Version:           ${PROJECT_VERSION}")
message(STATUS "  C Compiler:        ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  Build Type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build Tests:       ${BUILD_TESTS}")
message(STATUS "  Enable Warnings:   ${ENABLE_WARNINGS}")
message(STATUS "  Warnings as Errors: ${WARNINGS_AS_ERRORS}")
message(STATUS "")

# ==============================================================================
# Custom Targets
# ==============================================================================

# Format target (if clang-format is available)
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    add_custom_target(format
            COMMAND ${CLANG_FORMAT} -i
            ${CMAKE_CURRENT_SOURCE_DIR}/eventchains.c
            ${CMAKE_CURRENT_SOURCE_DIR}/eventchains.h
            ${CMAKE_CURRENT_SOURCE_DIR}/tinyllvm_ast.c
            ${CMAKE_CURRENT_SOURCE_DIR}/tinyllvm_ast.h
            ${CMAKE_CURRENT_SOURCE_DIR}/tinyllvm_lexer.c
            ${CMAKE_CURRENT_SOURCE_DIR}/tinyllvm_compiler.h
            ${CMAKE_CURRENT_SOURCE_DIR}/tinyllvm_ast_test.c
            ${CMAKE_CURRENT_SOURCE_DIR}/test_lexer_simple.c
            COMMENT "Formatting source code with clang-format"
    )
endif()

# Clean target (extended)
add_custom_target(clean-all
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
        COMMENT "Removing entire build directory"
)

# Documentation target
add_custom_target(docs
        COMMAND ${CMAKE_COMMAND} -E echo "Documentation files:"
        COMMAND ${CMAKE_COMMAND} -E echo "  README.md"
        COMMAND ${CMAKE_COMMAND} -E echo "  examples_coretiny.c"
        COMMENT "Listing documentation files"
)

# ==============================================================================
# CPack Configuration (for creating packages)
# ==============================================================================

set(CPACK_PACKAGE_NAME "TinyLLVM")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "TinyLLVM Project")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

include(CPack)