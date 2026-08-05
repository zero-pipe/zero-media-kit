# Vendored librtsp (ireader/media-server)
# Phase 1: SDP parser (always)
# Phase 2+: rtsp-muxer, rtp-sender, sdp-payload (ZMS RTSP egress)
# NOT included: rtsp-server, rtsp-client, aio — ZMS keeps L5 session control

set(LIBRTSP_ROOT "${CMAKE_CURRENT_LIST_DIR}")

set(LIBRTSP_SOURCES
    "${LIBRTSP_ROOT}/source/base64.c"
    "${LIBRTSP_ROOT}/source/sdp.c"
    "${LIBRTSP_ROOT}/source/sdp-a-rtpmap.c"
    "${LIBRTSP_ROOT}/source/sdp-a-fmtp.c"
    "${LIBRTSP_ROOT}/source/sdp-options.c"
    "${LIBRTSP_ROOT}/source/sdp/sdp-payload.c"
    "${LIBRTSP_ROOT}/source/sdp/sdp-h264.c"
    "${LIBRTSP_ROOT}/source/sdp/sdp-h265.c"
    "${LIBRTSP_ROOT}/source/sdp/sdp-h266.c"
    "${LIBRTSP_ROOT}/source/sdp/sdp-aac.c"
    "${LIBRTSP_ROOT}/source/sdp/sdp-mpeg2.c"
    "${LIBRTSP_ROOT}/source/sdp/sdp-mpeg4.c"
    "${LIBRTSP_ROOT}/source/sdp/sdp-g7xx.c"
    "${LIBRTSP_ROOT}/source/sdp/sdp-vpx.c"
    "${LIBRTSP_ROOT}/source/sdp/sdp-av1.c"
    "${LIBRTSP_ROOT}/source/sdp/sdp-opus.c"
    "${LIBRTSP_ROOT}/source/sdp/sdp-fmtp-load.c"
    "${LIBRTSP_ROOT}/source/utils/rtp-sender.c"
    "${LIBRTSP_ROOT}/source/utils/rtsp-muxer.c"
    "${LIBRTSP_ROOT}/source/client/rtp-over-rtsp.c"
)

add_library(librtsp STATIC ${LIBRTSP_SOURCES})
target_include_directories(librtsp
    PUBLIC "${LIBRTSP_ROOT}/include"
    PRIVATE "${LIBRTSP_ROOT}/source"
    PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../libflv/include"
    PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../libmov/include"
    PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../libmkv/include"
    PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../libmpeg/include"
    PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../librtp/include"
)
target_compile_definitions(librtsp PRIVATE ZMK_LIBRTSP=1)

target_link_libraries(librtsp PUBLIC librtp libflv libmpeg libmov libmkv)

if (WIN32)
    target_compile_definitions(librtsp PRIVATE OS_WINDOWS=1)
endif ()
if (MSVC)
    target_compile_definitions(librtsp PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(librtsp PRIVATE /wd4267 /wd4101 /wd4828)
endif ()

set_target_properties(librtsp PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)
