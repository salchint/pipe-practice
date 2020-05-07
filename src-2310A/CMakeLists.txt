
# Add CPP Check
include(CppcheckTargets)
add_cppcheck_sources(test UNUSED_FUNCTIONS STYLE POSSIBLE_ERRORS FORCE)

file(
    GLOB
    headers
    *.h
)

file(
    GLOB
    sources
    *.c
)

add_executable(
    2310A
    ${sources}
    ${headers}
)

install(
  TARGETS 2310A
    DESTINATION lib
)

install(
    FILES ${headers}
    DESTINATION include/${CMAKE_PROJECT_NAME}
)
