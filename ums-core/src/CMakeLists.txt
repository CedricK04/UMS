# Define the library sources
set(UMS_CORE_SOURCES
    ums_core.c
    # Add more source files here
)

set(UMS_CORE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/ums/ums_core.h
        ../include/ums/error.h
        ../include/ums/datatype.h
        ../include/ums/triple_buffer.h
        # Add more headers here
)

# Create library target
if(UMS_BUILD_SHARED_LIBS)
    add_library(ums-core SHARED ${UMS_CORE_SOURCES} ${UMS_CORE_HEADERS})
else()
    add_library(ums-core STATIC ${UMS_CORE_SOURCES} ${UMS_CORE_HEADERS})
endif()

# Add an alias for consistent naming
add_library(ums::core ALIAS ums-core)

# Include directories
target_include_directories(ums-core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})

# Compiler flags
target_compile_options(ums-core PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<C_COMPILER_ID:MSVC>:/W4>)

# Set properties
set_target_properties(ums-core PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${UMS_CORE_HEADERS}")