
# Add CPP Check
include(CppcheckTargets)
add_cppcheck_sources(test UNUSED_FUNCTIONS STYLE POSSIBLE_ERRORS FORCE)

file(
    GLOB
    headers
    *.hpp
)

file(
    GLOB
    sources
    *.cpp
)

add_library(
    pipe-practice
    SHARED
    ${sources}
    ${headers}
)

install(
    TARGETS pipe-practice
    DESTINATION lib
)

install(
    FILES ${headers}
    DESTINATION include/${CMAKE_PROJECT_NAME}
)
