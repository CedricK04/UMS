# Fetch GoogleTest with caching
include(FetchContent)

# Set FetchContent to use a shared base directory
set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/_deps" CACHE PATH "FetchContent base directory")

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Don't build gmock if you don't need it
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

# Make GoogleTest available
FetchContent_MakeAvailable(googletest)

# Enable GoogleTest integration with CTest
include(GoogleTest)

# Define test sources
set(TEST_SOURCES
    test_sampling.cpp
    # Add more test files here
)

# Create test executable
add_executable(ums_core_tests ${TEST_SOURCES})

target_link_libraries(ums_core_tests
    PRIVATE
        ums::core
        GTest::gtest_main)

target_include_directories(ums_core_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Discover tests
gtest_discover_tests(ums_core_tests)

# Valgrind integration
if(UMS_ENABLE_VALGRIND)
    find_program(VALGRIND_EXECUTABLE valgrind)
    
    if(VALGRIND_EXECUTABLE)
        message(STATUS "Valgrind found: ${VALGRIND_EXECUTABLE}")
        
        # Add valgrind test target
        add_test(NAME valgrind_memcheck
            COMMAND ${VALGRIND_EXECUTABLE}
                --leak-check=full
                --show-leak-kinds=all
                --track-origins=yes
                --error-exitcode=1
                $<TARGET_FILE:ums_core_tests>)
        
        set_tests_properties(valgrind_memcheck PROPERTIES
            LABELS "valgrind;memcheck")
    else()
        message(WARNING "Valgrind not found. Memory checking disabled.")
    endif()
endif()