# Vendored librtmp (ireader/media-server): RTMP server chunk/handshake/netstream.
# ZMS session layer: src/rtmp/rtmp_session*.c wires handlers to media_source.

set(LIBRTMP_ROOT "${CMAKE_CURRENT_LIST_DIR}")

set(LIBRTMP_SOURCES
    "${LIBRTMP_ROOT}/source/rtmp-chunk-header.c"
    "${LIBRTMP_ROOT}/source/rtmp-chunk-read.c"
    "${LIBRTMP_ROOT}/source/rtmp-chunk-write.c"
    "${LIBRTMP_ROOT}/source/rtmp-control-message.c"
    "${LIBRTMP_ROOT}/source/rtmp-control-handler.c"
    "${LIBRTMP_ROOT}/source/rtmp-event.c"
    "${LIBRTMP_ROOT}/source/rtmp-handshake.c"
    "${LIBRTMP_ROOT}/source/rtmp-handler.c"
    "${LIBRTMP_ROOT}/source/rtmp-invoke-handler.c"
    "${LIBRTMP_ROOT}/source/rtmp-netconnection.c"
    "${LIBRTMP_ROOT}/source/rtmp-netstream.c"
    "${LIBRTMP_ROOT}/source/rtmp-server.c"
    "${CMAKE_CURRENT_LIST_DIR}/../libflv/source/amf3.c"
)

add_library(librtmp STATIC ${LIBRTMP_SOURCES})
target_include_directories(librtmp
    PUBLIC "${LIBRTMP_ROOT}/include"
    PRIVATE "${LIBRTMP_ROOT}/source"
    PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../libflv/include"
)
target_compile_definitions(librtmp PRIVATE ZMK_LIBRTMP=1)

if (WIN32)
    target_compile_definitions(librtmp PRIVATE OS_WINDOWS=1)
endif ()
if (MSVC)
    target_compile_definitions(librtmp PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(librtmp PRIVATE /wd4267 /wd4101 /wd4828)
endif ()

set_target_properties(librtmp PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)
