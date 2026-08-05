# Vendored librtp (ireader/media-server): RTP queue + payload pack/unpack + RTP/RTCP session (for librtsp rtsp_muxer).
# Sync: https://github.com/ireader/media-server/tree/master/librtp

set(LIBRTP_ROOT "${CMAKE_CURRENT_LIST_DIR}")

set(LIBRTP_SOURCES
    "${LIBRTP_ROOT}/source/rtp.c"
    "${LIBRTP_ROOT}/source/rtp-time.c"
    "${LIBRTP_ROOT}/source/rtp-ssrc.c"
    "${LIBRTP_ROOT}/source/rtp-member.c"
    "${LIBRTP_ROOT}/source/rtp-member-list.c"
    "${LIBRTP_ROOT}/source/rtcp.c"
    "${LIBRTP_ROOT}/source/rtcp-interval.c"
    "${LIBRTP_ROOT}/source/rtcp-sr.c"
    "${LIBRTP_ROOT}/source/rtcp-rr.c"
    "${LIBRTP_ROOT}/source/rtcp-bye.c"
    "${LIBRTP_ROOT}/source/rtcp-app.c"
    "${LIBRTP_ROOT}/source/rtcp-psfb.c"
    "${LIBRTP_ROOT}/source/rtcp-rtpfb.c"
    "${LIBRTP_ROOT}/source/rtcp-sdec.c"
    "${LIBRTP_ROOT}/source/rtcp-xr.c"
    "${LIBRTP_ROOT}/source/rtp-demuxer.c"
    "${LIBRTP_ROOT}/source/rtp-packet.c"
    "${LIBRTP_ROOT}/source/rtp-profile.c"
    "${LIBRTP_ROOT}/source/rtp-queue.c"
    "${LIBRTP_ROOT}/payload/rtp-payload.c"
    "${LIBRTP_ROOT}/payload/rtp-payload-helper.c"
    "${LIBRTP_ROOT}/payload/rtp-pack.c"
    "${LIBRTP_ROOT}/payload/rtp-unpack.c"
    "${LIBRTP_ROOT}/payload/rtp-h264-bitstream.c"
    "${LIBRTP_ROOT}/payload/rtp-h264-pack.c"
    "${LIBRTP_ROOT}/payload/rtp-h264-unpack.c"
    "${LIBRTP_ROOT}/payload/rtp-h265-pack.c"
    "${LIBRTP_ROOT}/payload/rtp-h265-unpack.c"
    "${LIBRTP_ROOT}/payload/rtp-h266-pack.c"
    "${LIBRTP_ROOT}/payload/rtp-h266-unpack.c"
    "${LIBRTP_ROOT}/payload/rtp-mpeg4-generic-pack.c"
    "${LIBRTP_ROOT}/payload/rtp-mpeg4-generic-unpack.c"
    "${LIBRTP_ROOT}/payload/rtp-mp4a-latm-pack.c"
    "${LIBRTP_ROOT}/payload/rtp-mp4a-latm-unpack.c"
    "${LIBRTP_ROOT}/payload/rtp-mp4v-es-pack.c"
    "${LIBRTP_ROOT}/payload/rtp-mp4v-es-unpack.c"
    "${LIBRTP_ROOT}/payload/rtp-mpeg1or2es-pack.c"
    "${LIBRTP_ROOT}/payload/rtp-mpeg1or2es-unpack.c"
    "${LIBRTP_ROOT}/payload/rtp-ts-pack.c"
    "${LIBRTP_ROOT}/payload/rtp-ts-unpack.c"
    "${LIBRTP_ROOT}/payload/rtp-ps-unpack.c"
    "${LIBRTP_ROOT}/payload/rtp-vp8-pack.c"
    "${LIBRTP_ROOT}/payload/rtp-vp8-unpack.c"
    "${LIBRTP_ROOT}/payload/rtp-vp9-pack.c"
    "${LIBRTP_ROOT}/payload/rtp-vp9-unpack.c"
    "${LIBRTP_ROOT}/payload/rtp-av1-pack.c"
    "${LIBRTP_ROOT}/payload/rtp-av1-unpack.c"
)

add_library(librtp STATIC ${LIBRTP_SOURCES})
target_include_directories(librtp
    PUBLIC "${LIBRTP_ROOT}/include"
    PRIVATE "${LIBRTP_ROOT}/payload"
)
if (WIN32)
    target_compile_definitions(librtp PRIVATE OS_WINDOWS)
endif ()
target_compile_definitions(librtp PRIVATE RTP_MPEG4_GENERIC_SKIP_ADTS)
if (MSVC)
    target_compile_definitions(librtp PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(librtp PRIVATE /wd4267 /wd4101 /wd4828 /wd4996)
endif ()

set_target_properties(librtp PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)
