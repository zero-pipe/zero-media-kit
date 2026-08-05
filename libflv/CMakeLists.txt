# Vendored libflv (ireader/media-server): codec helpers + FLV mux/demux/header.
# Phase 3: flv-header, flv-muxer, flv-demuxer (H.264/AAC primary; other codecs compile for demuxer completeness).

set(LIBFLV_ROOT "${CMAKE_CURRENT_LIST_DIR}")

set(LIBFLV_SOURCES
    ${LIBFLV_ROOT}/source/mpeg4-avc.c
    ${LIBFLV_ROOT}/source/mpeg4-annexbtomp4.c
    ${LIBFLV_ROOT}/source/mpeg4-mp4toannexb.c
    ${LIBFLV_ROOT}/source/mpeg4-aac.c
    ${LIBFLV_ROOT}/source/mpeg4-aac-asc.c
    ${LIBFLV_ROOT}/source/mpeg4-hevc.c
    ${LIBFLV_ROOT}/source/hevc-annexbtomp4.c
    ${LIBFLV_ROOT}/source/hevc-mp4toannexb.c
    ${LIBFLV_ROOT}/source/mpeg4-vvc.c
    ${LIBFLV_ROOT}/source/vvc-annexbtomp4.c
    ${LIBFLV_ROOT}/source/vvc-mp4toannexb.c
    ${LIBFLV_ROOT}/source/aom-av1.c
    ${LIBFLV_ROOT}/source/avswg-avs3.c
    ${LIBFLV_ROOT}/source/opus-head.c
    ${LIBFLV_ROOT}/source/xiph-flac.c
    ${LIBFLV_ROOT}/source/mp3-header.c
    ${LIBFLV_ROOT}/source/riff-acm.c
    ${LIBFLV_ROOT}/source/webm-vpx.c
    ${LIBFLV_ROOT}/source/amf0.c
    ${LIBFLV_ROOT}/source/flv-header.c
    ${LIBFLV_ROOT}/source/flv-writer.c
    ${LIBFLV_ROOT}/source/flv-muxer.c
    ${LIBFLV_ROOT}/source/flv-demuxer.c
    ${LIBFLV_ROOT}/source/flv-demuxer-script.c
    ${LIBFLV_ROOT}/source/flv-reader.c
)

add_library(libflv STATIC ${LIBFLV_SOURCES})
target_include_directories(libflv
    PUBLIC "${LIBFLV_ROOT}/include"
)
target_compile_definitions(libflv PRIVATE ZMK_LIBFLV=1)

if (MSVC)
    target_compile_definitions(libflv PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(libflv PRIVATE /wd4267 /wd4101 /wd4828 /wd4996)
endif ()

set_target_properties(libflv PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)
