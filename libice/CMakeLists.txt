# Vendored libice (ireader/sdk): ICE/STUN/TURN for ZMS WebRTC.
# Layout: libice/, include/, source/ (digest, darray). Timers use ZTK via webrtc_ice_port.c.

set(LIBICE_ROOT "${CMAKE_CURRENT_LIST_DIR}")

set(LIBICE_SOURCES
    ${LIBICE_ROOT}/libice/src/ice-agent.c
    ${LIBICE_ROOT}/libice/src/ice-candidate.c
    ${LIBICE_ROOT}/libice/src/ice-checklist.c
    ${LIBICE_ROOT}/libice/src/ice-gather.c
    ${LIBICE_ROOT}/libice/src/ice-internal.c
    ${LIBICE_ROOT}/libice/src/ice-stream.c
    ${LIBICE_ROOT}/libice/src/stun-agent.c
    ${LIBICE_ROOT}/libice/src/stun-attr.c
    ${LIBICE_ROOT}/libice/src/stun-auth.c
    ${LIBICE_ROOT}/libice/src/stun-client.c
    ${LIBICE_ROOT}/libice/src/stun-message.c
    ${LIBICE_ROOT}/libice/src/stun-request.c
    ${LIBICE_ROOT}/libice/src/stun-response.c
    ${LIBICE_ROOT}/libice/src/stun-server.c
    ${LIBICE_ROOT}/libice/src/stun-transid.c
    ${LIBICE_ROOT}/libice/src/turn-agent.c
    ${LIBICE_ROOT}/libice/src/turn-allocate.c
    ${LIBICE_ROOT}/libice/src/turn-client.c
    ${LIBICE_ROOT}/libice/src/turn-server.c
    ${LIBICE_ROOT}/source/darray.c
    ${LIBICE_ROOT}/source/digest/md5.c
    ${LIBICE_ROOT}/source/digest/sha.c
    ${LIBICE_ROOT}/source/digest/sha1.c
    ${LIBICE_ROOT}/source/digest/sha224-256.c
    ${LIBICE_ROOT}/source/digest/sha384-512.c
    ${LIBICE_ROOT}/source/digest/crc32.c
    ${LIBICE_ROOT}/source/digest/hmac.c
)

add_library(libice STATIC ${LIBICE_SOURCES})
target_include_directories(libice
    PUBLIC
        ${LIBICE_ROOT}/libice/include
        ${LIBICE_ROOT}/include
)
target_compile_definitions(libice PRIVATE ZMK_LIBICE=1)

if(WIN32)
    target_compile_definitions(libice PRIVATE OS_WINDOWS=1)
endif()

if(MSVC)
    set(_zms_libice_forced "${LIBICE_ROOT}/../../../cmake/libice_forced.h")
    target_compile_options(libice PRIVATE /wd4018 /wd4996 /wd4244)
    target_compile_options(libice PRIVATE "/FI${_zms_libice_forced}")
else()
    target_compile_options(libice PRIVATE -w)
endif()

set_target_properties(libice PROPERTIES
    OUTPUT_NAME libice
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)
