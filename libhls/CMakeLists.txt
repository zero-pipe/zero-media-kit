# Vendored libhls (ireader/media-server): m3u8 playlist + TS segmenter over libmpeg.
# Phase 1 scope: hls-media.c + hls-m3u8.c (no parser/fmp4).

set(LIBHLS_ROOT "${CMAKE_CURRENT_LIST_DIR}")

set(LIBHLS_SOURCES
    "${LIBHLS_ROOT}/source/hls-media.c"
    "${LIBHLS_ROOT}/source/hls-m3u8.c"
    "${LIBHLS_ROOT}/source/hls-fmp4.c"
)

add_library(libhls STATIC ${LIBHLS_SOURCES})
target_include_directories(libhls
    PUBLIC "${LIBHLS_ROOT}/include"
    PRIVATE "${LIBHLS_ROOT}/source"
    PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../libmov/include"
)
target_link_libraries(libhls PUBLIC libmpeg libmov)

if (MSVC)
    target_compile_definitions(libhls PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(libhls PRIVATE /wd4267 /wd4101 /wd4828)
endif ()

set_target_properties(libhls PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)
