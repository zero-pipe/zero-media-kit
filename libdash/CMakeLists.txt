set(LIBDASH_ROOT "${CMAKE_CURRENT_LIST_DIR}")

# ZMS live DASH only needs MPD + fMP4 mux (dash-mpd.c).
set(LIBDASH_SOURCES
    "${LIBDASH_ROOT}/src/dash-mpd.c"
    "${LIBDASH_ROOT}/src/xs-duration.c"
)

add_library(libdash STATIC ${LIBDASH_SOURCES})
target_include_directories(libdash
    PUBLIC "${LIBDASH_ROOT}/include"
    PRIVATE "${LIBDASH_ROOT}/src"
    PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../libflv/include"
    PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../libmov/include"
)
target_link_libraries(libdash PUBLIC libmov libflv)

if (MSVC)
    target_compile_definitions(libdash PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(libdash PRIVATE /wd4267 /wd4101 /wd4828)
endif ()

set_target_properties(libdash PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)
