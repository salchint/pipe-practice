

#find_package(GTest REQUIRED)

#include_directories(
    #${CMAKE_SOURCE_DIR}/src
    #${GTEST_INCLUDE_DIRS}
#)

# Download and unpack googletest at configure time
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/googletest-download"
)
execute_process(COMMAND "${CMAKE_COMMAND}" --build .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/googletest-download"
)

# Prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This adds the following targets:
# gtest, gtest_main, gmock and gmock_main
add_subdirectory("${CMAKE_BINARY_DIR}/../tests/googletest-src"
                 "${CMAKE_BINARY_DIR}/../tests/googletest-build"
)

# The gtest/gmock targets carry header search path dependencies
# automatically when using CMake 2.8.11 or later. Otherwise we
# have to add them here ourselves.
if(CMAKE_VERSION VERSION_LESS 2.8.11)
    include_directories("${gtest_SOURCE_DIR}/include"
                        "${gmock_SOURCE_DIR}/include"
    )
endif()

# Now simply link your own targets against gtest, gmock,
# etc. as appropriate

file(
    GLOB
    test_files
    *.cpp
)

add_executable(Tests-pipe-practice
    ${test_files}
)

target_link_libraries(Tests-pipe-practice
    pipe-practice
    #${GTEST_BOTH_LIBRARIES}
    gtest
    gtest_main
    pthread
)
