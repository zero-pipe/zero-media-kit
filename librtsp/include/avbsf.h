#ifndef _avbsf_h_
#define _avbsf_h_

#include <stdint.h>

/* Stub for ZMS build: upstream rtsp-muxer links ireader/avcodec avbsf when enabled.
 * BSF paths are currently disabled in rtsp-muxer.c (avbsf_find commented out). */

typedef struct avbsf_t {
    void *(*create)(const void *extra, int size,
        void (*onpacket)(void *param, int64_t pts, int64_t dts, const uint8_t *data, int bytes, int flags),
        void *param);
    int (*input)(void *filter, int64_t pts, int64_t dts, const uint8_t *data, int bytes);
    void (*destroy)(void **filter);
} avbsf_t;

#endif
