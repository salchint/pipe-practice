
# Add CPP Check
include(CppcheckTargets)
add_cppcheck_sources(test UNUSED_FUNCTIONS STYLE POSSIBLE_ERRORS FORCE)

file(
    GLOB
    headers
    *.h
    ../inc/*.h
)

file(
    GLOB
    sources
    *.c
    ../inc/*.c
)

add_executable(
    2310B
    ${sources}
    ${headers}
)
target_link_libraries(2310B m)

install(
  TARGETS 2310B
    DESTINATION lib
)

install(
    FILES ${headers}
    DESTINATION include/${CMAKE_PROJECT_NAME}
)
