# Vendored libmpeg (ireader/media-server), used by ZLMK HLS TS muxer.
# Sources live under this directory so c_version can build standalone.

set(LIBMPEG_ROOT "${CMAKE_CURRENT_LIST_DIR}")

file(GLOB LIBMPEG_SOURCES "${LIBMPEG_ROOT}/source/*.c")

add_library(libmpeg STATIC ${LIBMPEG_SOURCES})
target_include_directories(libmpeg
    PUBLIC "${LIBMPEG_ROOT}/include"
    PRIVATE "${LIBMPEG_ROOT}/source"
)

if (MSVC)
    target_compile_definitions(libmpeg PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(libmpeg PRIVATE /wd4267 /wd4101 /wd4828)
endif ()

set_target_properties(libmpeg PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)
