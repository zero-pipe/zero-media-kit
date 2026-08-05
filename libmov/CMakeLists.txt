# Vendored libmov (ireader/media-server), MP4/fMP4 demux for ZLMK VOD.
set(LIBMOV_ROOT "${CMAKE_CURRENT_LIST_DIR}")

file(GLOB LIBMOV_SOURCES "${LIBMOV_ROOT}/source/*.c")

add_library(libmov STATIC ${LIBMOV_SOURCES})
target_include_directories(libmov
    PUBLIC "${LIBMOV_ROOT}/include"
    PRIVATE "${LIBMOV_ROOT}/source"
)

if (MSVC)
    target_compile_definitions(libmov PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(libmov PRIVATE /wd4267 /wd4101 /wd4828)
endif ()

set_target_properties(libmov PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)
