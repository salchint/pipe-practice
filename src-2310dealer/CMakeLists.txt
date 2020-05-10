
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
    2310dealer
    ${sources}
    ${headers}
)
target_link_libraries(2310dealer m)

install(
  TARGETS 2310dealer
    DESTINATION lib
)

install(
    FILES ${headers}
    DESTINATION include/${CMAKE_PROJECT_NAME}
)
