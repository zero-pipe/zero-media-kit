#include "dash-mpd.h"
#include "dash-proto.h"
#include "mov-format.h"
#include "fmp4-writer.h"
#include "mpeg4-hevc.h"
#include "mpeg4-vvc.h"
#include "mpeg4-avc.h"
#include "mpeg4-aac.h"
#include "aom-av1.h"
#include "webm-vpx.h"
#include "list.h"
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#define N_TRACK 8
#define N_NAME 128
#define N_COUNT 24

#define N_SEGMENT (1 * 1024 * 1024)
#define N_FILESIZE (100 * 1024 * 1024) // 100M
/** 直播最短分片时长(ms)，避免碎段与音画 MPD 错位 */
#define DASH_LIVE_MIN_SEG_MS 1500

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static int dash_mpd_is_video_object(uint8_t object);
static void dash_vpx_codecs(uint8_t object, const void* extra, size_t extra_size, char* codecs, size_t cap);
static int dash_mpd_is_audio_object(uint8_t object);

static int dash_mpd_is_audio_object(uint8_t object)
{
	return MOV_OBJECT_AAC == object || MOV_OBJECT_OPUS == object;
}

static struct tm *dash_gmtime_safe(const time_t *t, struct tm *out)
{
	if (!t || !out)
		return NULL;
#if defined(_WIN32)
	return gmtime_s(out, t) == 0 ? out : NULL;
#else
	return gmtime_r(t, out) ? out : NULL;
#endif
}

static void dash_format_utc(time_t t, char *buf, size_t cap)
{
	struct tm tm_buf;

	if (!buf || cap == 0)
		return;
	if (!dash_gmtime_safe(&t, &tm_buf)) {
		snprintf(buf, cap, "1970-01-01T00:00:00Z");
		return;
	}
	strftime(buf, cap, "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
}

struct dash_segment_t
{
	struct list_head link;
	int64_t timestamp;
	int64_t duration;
};

struct dash_adaptation_set_t
{
	fmp4_writer_t* fmp4;
	char prefix[N_NAME];

	uint8_t* ptr;
	size_t bytes;
	size_t capacity;
	size_t offset;
	size_t maxsize; // max bytes per mp4 file

	int64_t pts;
	int64_t dts;
	int64_t dts_last;
	int64_t raw_bytes;
	int bitrate;
	int track; // MP4 track id
	int setid; // dash adapation set id
	
	int seq;
	uint8_t object;
	char name[8]; // H264/H265/H266/AAC
	char codecs[64]; // avc1.x.x.x

	union
	{
		struct
		{
			int width;
			int height;
			int frame_rate;
		} video;

		struct
		{
			uint8_t profile; // AAC profile
			int channel;
			int sample_bit;
			int sample_rate;
		} audio;
	} u;

	size_t count;
	struct list_head root; // segments
};

struct dash_mpd_t
{
	int flags;
	time_t time;
	int64_t duration;
	int64_t max_segment_duration;

	dash_mpd_segment handler;
	void* param;

	union
	{
		struct mpeg4_avc_t avc;
		struct mpeg4_hevc_t hevc;
		struct mpeg4_vvc_t vvc;

		struct mpeg4_aac_t aac;
	} v;

	int count; // adaptation set count
	struct dash_adaptation_set_t tracks[N_TRACK];
};

static int mov_buffer_read(void* param, void* data, uint64_t bytes)
{
	struct dash_adaptation_set_t* dash;
	dash = (struct dash_adaptation_set_t*)param;
	if (dash->offset + bytes > dash->bytes)
		return -E2BIG;
	memcpy(data, dash->ptr + dash->offset, (size_t)bytes);
	return 0;
}

static int mov_buffer_write(void* param, const void* data, uint64_t bytes)
{
	void* ptr;
	size_t capacity;
	struct dash_adaptation_set_t* dash;
	dash = (struct dash_adaptation_set_t*)param;
	if (dash->offset + bytes > dash->maxsize)
		return -E2BIG;

	if (dash->offset + (size_t)bytes > dash->capacity)
	{
		capacity = dash->offset + (size_t)bytes + N_SEGMENT;
		capacity = capacity > dash->maxsize ? dash->maxsize : capacity;
		ptr = realloc(dash->ptr, capacity);
		if (NULL == ptr)
			return -ENOMEM;
		dash->ptr = ptr;
		dash->capacity = capacity;
	}

	memcpy(dash->ptr + dash->offset, data, (size_t)bytes);
	dash->offset += (size_t)bytes;
	if (dash->offset > dash->bytes)
		dash->bytes = dash->offset;
	return 0;
}

static int mov_buffer_seek(void* param, int64_t offset)
{
	struct dash_adaptation_set_t* dash;
	dash = (struct dash_adaptation_set_t*)param;
	if ((offset >= 0 ? offset : -offset) >= dash->maxsize)
		return -E2BIG;
	dash->offset = (size_t)(offset >= 0 ? offset : (dash->maxsize+offset));
	return 0;
}

static int64_t mov_buffer_tell(void* param)
{
	return (int64_t)((struct dash_adaptation_set_t*)param)->offset;
}

static struct mov_buffer_t s_io = {
	mov_buffer_read,
	mov_buffer_write,
	mov_buffer_seek,
	mov_buffer_tell,
};

static int dash_adaptation_set_segment(struct dash_mpd_t* mpd, struct dash_adaptation_set_t* track, int64_t name_ts)
{
	int r;
	char name[N_NAME + 32];
	struct list_head *link;
	struct dash_segment_t* new_seg;
	struct dash_segment_t* evict;
	int64_t seg_t;

	r = fmp4_writer_save_segment(track->fmp4);
	if (0 != r)
		return r;
	if (track->bytes < 1)
		return 0;

	seg_t = name_ts >= 0 ? name_ts : track->dts;

	new_seg = (struct dash_segment_t*)calloc(1, sizeof(*new_seg));
    if(!new_seg)
        return -1; // ENOMEM
	new_seg->timestamp = seg_t;
	new_seg->duration = track->dts_last > track->dts ? (track->dts_last - track->dts) : 0;
	if (new_seg->duration <= 0)
		new_seg->duration = 1;

	if (dash_mpd_is_audio_object(track->object))
		snprintf(name, sizeof(name) - 1, "%s-%" PRId64 ".m4a", track->prefix, seg_t);
	else
		snprintf(name, sizeof(name) - 1, "%s-%" PRId64 ".m4v", track->prefix, seg_t);

	/* Timeline insert/evict before handler so MPD refresh sees committed window. */
	list_insert_after(&new_seg->link, track->root.prev);
	track->count += 1;
	if (DASH_DYNAMIC == mpd->flags && track->count > N_COUNT)
	{
		link = track->root.next;
		list_remove(link);
		evict = list_entry(link, struct dash_segment_t, link);
		free(evict);
		--track->count;
	}

	r = mpd->handler(mpd->param, track->setid, track->ptr, track->bytes, track->pts, track->dts, new_seg->duration, name);
	if (0 != r)
	{
		list_remove(&new_seg->link);
		free(new_seg);
		if (track->count > 0)
			--track->count;
		return r;
	}
	return 0;
}

static int dash_mpd_flush_track(struct dash_mpd_t* mpd, struct dash_adaptation_set_t* track, int64_t align_name_ts)
{
	int r = 0;
	int64_t origin_dts;

	if (!track || !track->raw_bytes)
		return 0;

	origin_dts = track->dts;
	r = dash_adaptation_set_segment(mpd, track, align_name_ts);
	mpd->max_segment_duration = MAX(track->dts_last - origin_dts, mpd->max_segment_duration);
	if (track->dts_last > origin_dts)
		track->bitrate = MAX(track->bitrate, (int)(track->raw_bytes * 1000 / (track->dts_last - origin_dts) * 8));

	track->pts = INT64_MIN;
	track->dts = INT64_MIN;
	track->raw_bytes = 0;
	track->offset = 0;
	track->bytes = 0;
	return r;
}

static int dash_mpd_flush(struct dash_mpd_t* mpd)
{
	int i, r;

	for (r = i = 0; i < mpd->count && 0 == r; i++)
		r = dash_mpd_flush_track(mpd, &mpd->tracks[i], -1);
	return r;
}

struct dash_mpd_t* dash_mpd_create(int flags, dash_mpd_segment segment, void* param)
{
	struct dash_mpd_t* mpd;
	mpd = (struct dash_mpd_t*)calloc(1, sizeof(*mpd));
	if (mpd)
	{
		mpd->flags = flags;
		mpd->handler = segment;
		mpd->param = param;
		mpd->time = time(NULL);
	}
	return mpd;
}

void dash_mpd_destroy(struct dash_mpd_t* mpd)
{
	int i;
	struct list_head *p, *n;
	struct dash_segment_t *seg;
	struct dash_adaptation_set_t* track;

	dash_mpd_flush(mpd);

	for (i = 0; i < mpd->count; i++)
	{
		track = &mpd->tracks[i];

		if (track->ptr)
		{
			free(track->ptr);
			track->ptr = NULL;
		}

		list_for_each_safe(p, n, &track->root)
		{
			seg = list_entry(p, struct dash_segment_t, link);
			free(seg);
		}
	}

	free(mpd);
}

int dash_mpd_add_video_adaptation_set(struct dash_mpd_t* mpd, const char* prefix, uint8_t object, int width, int height, const void* extra_data, size_t extra_data_size)
{
	int r;
	char name[N_NAME + 16];
	struct dash_adaptation_set_t* track;

	r = (int)strlen(prefix);
	if (mpd->count + 1 >= N_TRACK || r >= N_NAME)
		return -1;
	if (MOV_OBJECT_VP8 != object && MOV_OBJECT_VP9 != object) {
		if (extra_data_size < 4)
			return -1;
		assert(((const uint8_t*)extra_data)[0] == 1); // configurationVersion
	}
	else if (extra_data_size < 8)
		return -1;

	assert(MOV_OBJECT_H264 == object || MOV_OBJECT_H265 == object || MOV_OBJECT_H266 == object ||
		MOV_OBJECT_AV1 == object || MOV_OBJECT_VP8 == object || MOV_OBJECT_VP9 == object);
	track = &mpd->tracks[mpd->count];
	memcpy(track->prefix, prefix, r);
	switch (object)
	{
	case MOV_OBJECT_VP8:
		snprintf(track->name, sizeof(track->name), "%s", "VP8");
		dash_vpx_codecs(object, extra_data, extra_data_size, track->codecs, sizeof(track->codecs));
		break;

	case MOV_OBJECT_VP9:
		snprintf(track->name, sizeof(track->name), "%s", "VP9");
		dash_vpx_codecs(object, extra_data, extra_data_size, track->codecs, sizeof(track->codecs));
		break;

	case MOV_OBJECT_H264:
		r = mpeg4_avc_decoder_configuration_record_load(extra_data, extra_data_size, &mpd->v.avc);
		if (r <= 0)
			return -EINVAL; // invalid extra_data
		snprintf(track->name, sizeof(track->name), "%s", "H264");
		mpeg4_avc_codecs(&mpd->v.avc, track->codecs, sizeof(track->codecs));
		break;

	case MOV_OBJECT_H265:
		r = mpeg4_hevc_decoder_configuration_record_load(extra_data, extra_data_size, &mpd->v.hevc);
		if (r <= 0)
			return -EINVAL; // invalid extra_data
		snprintf(track->name, sizeof(track->name), "%s", "H265");
		mpeg4_hevc_codecs(&mpd->v.hevc, track->codecs, sizeof(track->codecs));
		break;

	case MOV_OBJECT_H266:
		r = mpeg4_vvc_decoder_configuration_record_load(extra_data, extra_data_size, &mpd->v.vvc);
		if (r <= 0)
			return -EINVAL; // invalid extra_data
		snprintf(track->name, sizeof(track->name), "%s", "H266");
		mpeg4_vvc_codecs(&mpd->v.vvc, track->codecs, sizeof(track->codecs));
		break;

	case MOV_OBJECT_AV1:
		if (extra_data_size < 4)
			return -EINVAL;
		snprintf(track->name, sizeof(track->name), "%s", "AV1");
		{
			struct aom_av1_t av1;
			memset(&av1, 0, sizeof(av1));
			if (aom_av1_codec_configuration_record_load(extra_data, extra_data_size, &av1) > 0)
				aom_av1_codecs(&av1, track->codecs, sizeof(track->codecs));
			else
				snprintf(track->codecs, sizeof(track->codecs), "av01.0.05M.08");
		}
		break;

	default:
		assert(0);
		return -EINVAL;
	}

	LIST_INIT_HEAD(&track->root);
	track->setid = mpd->count++;
	track->object = object;
	track->bitrate = 0;
	track->u.video.width = width;
	track->u.video.height = height;
	track->u.video.frame_rate = 25;

	track->seq = 1;
	track->maxsize = N_FILESIZE;
	track->fmp4 = fmp4_writer_create(&s_io, track, MOV_FLAG_SEGMENT);
	if (!track->fmp4)
		return -1;
	track->track = fmp4_writer_add_video(track->fmp4, object, width, height, extra_data, extra_data_size);
	
	// save init segment file
	r = fmp4_writer_init_segment(track->fmp4);
	if (0 == r)
	{
		snprintf(name, sizeof(name) - 1, "%s-init.m4v", prefix);
		r = mpd->handler(mpd->param, mpd->count, track->ptr, track->bytes, 0, 0, 0, name);
	}

	track->bytes = 0;
	track->offset = 0;
	return 0 == r ? track->setid : r;
}

int dash_mpd_add_audio_adaptation_set(struct dash_mpd_t* mpd, const char* prefix, uint8_t object, int channel_count, int bits_per_sample, int sample_rate, const void* extra_data, size_t extra_data_size)
{
	int r;
	char name[N_NAME + 16];
	struct dash_adaptation_set_t* track;

	r = (int)strlen(prefix);
	if (mpd->count + 1 >= N_TRACK || extra_data_size < 2 || r >= N_NAME)
		return -1;

	assert(MOV_OBJECT_AAC == object || MOV_OBJECT_OPUS == object);
	track = &mpd->tracks[mpd->count];
	memcpy(track->prefix, prefix, r);
	if (MOV_OBJECT_AAC == object)
	{
		r = mpeg4_aac_audio_specific_config_load(extra_data, extra_data_size, &mpd->v.aac);
		if (r <= 0)
			return -EINVAL;
		snprintf(track->name, sizeof(track->name), "%s", "AAC");
		mpeg4_aac_codecs(&mpd->v.aac, track->codecs, sizeof(track->codecs));
	}
	else if (MOV_OBJECT_OPUS == object)
	{
		if (extra_data_size < 19 || memcmp(extra_data, "OpusHead", 8) != 0)
			return -EINVAL;
		snprintf(track->name, sizeof(track->name), "%s", "OPUS");
		snprintf(track->codecs, sizeof(track->codecs), "%s", "opus");
	}

	LIST_INIT_HEAD(&track->root);
	track->setid = mpd->count++;
	track->object = object;
	track->bitrate = 0;
	track->u.audio.channel = channel_count;
	track->u.audio.sample_bit = bits_per_sample;
	track->u.audio.sample_rate = sample_rate;
	track->u.audio.profile = ((const uint8_t*)extra_data)[0] >> 3;
	if(MOV_OBJECT_AAC == object && 31 == track->u.audio.profile)
		track->u.audio.profile = 32 + (((((const uint8_t*)extra_data)[0] & 0x07) << 3) | ((((const uint8_t*)extra_data)[1] >> 5) & 0x07));

	track->seq = 1;
	track->maxsize = N_FILESIZE;
	track->fmp4 = fmp4_writer_create(&s_io, track, MOV_FLAG_SEGMENT);
	if (!track->fmp4)
		return -1;
	track->track = fmp4_writer_add_audio(track->fmp4, object, channel_count, bits_per_sample, sample_rate, extra_data, extra_data_size);

	r = fmp4_writer_init_segment(track->fmp4);
	if (0 == r)
	{
		snprintf(name, sizeof(name) - 1, "%s-init.m4a", prefix);
		r = mpd->handler(mpd->param, mpd->count, track->ptr, track->bytes, 0, 0, 0, name);
	}

	track->bytes = 0;
	track->offset = 0;
	return 0 == r ? track->setid : r;
}

int dash_mpd_input(struct dash_mpd_t* mpd, int adapation, const void* data, size_t bytes, int64_t pts, int64_t dts, int flags)
{
	int r = 0;
	int i;
	struct dash_adaptation_set_t* track;
	if (adapation >= mpd->count || adapation < 0)
		return -1;

	track = &mpd->tracks[adapation];
	if (NULL == data || 0 == bytes)
	{
		r = dash_mpd_flush(mpd);
		mpd->duration += mpd->max_segment_duration;
	}
	else if ((MOV_AV_FLAG_KEYFREAME & flags) && dash_mpd_is_video_object(track->object))
	{
		/* Flush video on IDR; co-flush audio only after video segment is committed. */
		if (track->raw_bytes > 0)
		{
			int64_t seg_dur = track->dts_last > track->dts ? (track->dts_last - track->dts) : 0;

			if (seg_dur >= DASH_LIVE_MIN_SEG_MS)
			{
				int64_t video_seg_t = track->dts;
				int vr;

				vr = dash_mpd_flush_track(mpd, track, -1);
				if (0 == vr)
				{
					for (i = 0; i < mpd->count; i++)
					{
						struct dash_adaptation_set_t* a = &mpd->tracks[i];

						if (a == track || !dash_mpd_is_audio_object(a->object) || !a->raw_bytes)
							continue;
						r = dash_mpd_flush_track(mpd, a, video_seg_t);
					}
					mpd->duration += mpd->max_segment_duration;
				}
				else
					r = vr;
			}
		}
	}

	if (NULL == data || 0 == bytes)
		return r;

	if (0 == track->raw_bytes)
	{
		track->pts = pts;
		track->dts = dts;
	}
	track->dts_last = dts;
	track->raw_bytes += bytes;
	/* 切段仅由 dash_mpd_flush/save_segment 触发，禁止 fmp4 在 IDR 上再拆 moof */
	return fmp4_writer_write(track->fmp4, track->track, data, bytes, pts, dts,
		flags | MOV_AV_FLAG_SEGMENT_DISABLE);
}

static int dash_mpd_is_video_object(uint8_t object)
{
	return MOV_OBJECT_H264 == object || MOV_OBJECT_H265 == object || MOV_OBJECT_HEVC == object ||
		MOV_OBJECT_VVC == object || MOV_OBJECT_H266 == object || MOV_OBJECT_AV1 == object ||
		MOV_OBJECT_VP8 == object || MOV_OBJECT_VP9 == object;
}

static void dash_vpx_codecs(uint8_t object, const void* extra, size_t extra_size, char* codecs, size_t cap)
{
	struct webm_vpx_t vpx;
	const char* fourcc;

	memset(&vpx, 0, sizeof(vpx));
	vpx.profile = 0;
	vpx.level = 31;
	vpx.bit_depth = 8;
	if (extra && extra_size >= 8)
		webm_vpx_codec_configuration_record_load((const uint8_t*)extra, extra_size, &vpx);
	fourcc = (MOV_OBJECT_VP9 == object) ? "vp09" : "vp08";
	snprintf(codecs, cap, "%s.%02d.%02x.0%x", fourcc, vpx.profile, vpx.level, vpx.bit_depth);
}

static struct dash_adaptation_set_t* dash_mpd_find_video_track(struct dash_mpd_t* mpd)
{
	int i;

	for (i = 0; i < mpd->count; i++)
	{
		if (dash_mpd_is_video_object(mpd->tracks[i].object))
			return &mpd->tracks[i];
	}
	return NULL;
}

static int dash_mpd_track_has_timestamp(const struct dash_adaptation_set_t* track, int64_t t)
{
	struct list_head* link;
	struct dash_segment_t* seg;

	if (!track)
		return 0;
	list_for_each(link, &track->root)
	{
		seg = list_entry(link, struct dash_segment_t, link);
		if (seg->timestamp == t)
			return 1;
	}
	return 0;
}

// ISO/IEC 23009-1:2014(E) 5.4 Media Presentation Description updates (p67)
// 1. the value of MPD@id, if present, shall be the same in the original and the updated MPD;
// 2. the values of any Period@id attributes shall be the same in the original and the updated MPD, unless the containing Period element has been removed;
// 3. the values of any AdaptationSet@id attributes shall be the same in the original and the updated MPD unless the containing Period element has been removed;
size_t dash_mpd_playlist(struct dash_mpd_t* mpd, char* playlist, size_t bytes)
{
	// ISO/IEC 23009-1:2014(E)
	// G.2 Example for ISO Base media file format Live profile (141)
	static const char* s_mpd_dynamic =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<MPD\n"
		"    xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
		"    xmlns=\"urn:mpeg:dash:schema:mpd:2011\"\n"
		"    xsi:schemaLocation=\"urn:mpeg:DASH:schema:MPD:2011 DASH-MPD.xsd\"\n"
		"    type=\"dynamic\"\n"
		"    minimumUpdatePeriod=\"PT%uS\"\n"
		"    timeShiftBufferDepth=\"PT%uS\"\n"
		"    availabilityStartTime=\"%s\"\n"
		"    suggestedPresentationDelay=\"PT%uS\"\n"
		"    minBufferTime=\"PT%uS\"\n"
		"    publishTime=\"%s\"\n"
		"    profiles=\"urn:mpeg:dash:profile:isoff-live:2011\">\n";

	// ISO/IEC 23009-1:2014(E)
	// G.1 Example MPD for ISO Base media file format On Demand profile
	static const char* s_mpd_static =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<MPD\n"
		"    xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
		"    xmlns=\"urn:mpeg:dash:schema:mpd:2011\"\n"
		"    xsi:schemaLocation=\"urn:mpeg:DASH:schema:MPD:2011 DASH-MPD.xsd\"\n"
		"    type=\"static\"\n"
		"    mediaPresentationDuration=\"PT%uS\"\n"
		"    minBufferTime=\"PT%uS\"\n"
		"    profiles=\"urn:mpeg:dash:profile:isoff-on-demand:2011\">\n";

	static const char* s_h264 =
		"    <AdaptationSet contentType=\"video\" segmentAlignment=\"true\" bitstreamSwitching=\"true\">\n"
		"      <Representation id=\"%s\" mimeType=\"video/mp4\" codecs=\"%s\" width=\"%d\" height=\"%d\" frameRate=\"%d\" startWithSAP=\"1\" bandwidth=\"%d\">\n"
		"        <SegmentTemplate timescale=\"1000\" media=\"%s-$Time$.m4v\" initialization=\"%s-init.m4v\">\n"
		"          <SegmentTimeline>\n";

	static const char* s_aac =
		"    <AdaptationSet contentType=\"audio\" segmentAlignment=\"true\" bitstreamSwitching=\"true\">\n"
		"      <Representation id=\"%s\" mimeType=\"audio/mp4\" codecs=\"%s\" audioSamplingRate=\"%d\" startWithSAP=\"1\" bandwidth=\"%d\">\n"
		"		 <AudioChannelConfiguration schemeIdUri=\"urn:mpeg:dash:23003:3:audio_channel_configuration:2011\" value=\"%d\"/>\n"
		"        <SegmentTemplate timescale=\"1000\" media=\"%s-$Time$.m4a\" initialization=\"%s-init.m4a\">\n"
		"          <SegmentTimeline>\n";

	static const char* s_footer =
		"          </SegmentTimeline>\n"
		"        </SegmentTemplate>\n"
		"      </Representation>\n"
		"    </AdaptationSet>\n";

	int i;
	size_t n;
	time_t now;
	char publishTime[32];
	char availabilityStartTime[32];
	unsigned int minimumUpdatePeriod;
	unsigned int timeShiftBufferDepth;
	unsigned int suggestedDelay;
	struct dash_adaptation_set_t* track;
	struct dash_segment_t *seg;
	struct list_head *link, *next;

	now = time(NULL);
	/* Segment $Time$ uses mux DTS (ms from stream start); anchor AST to recorder start. */
	dash_format_utc(mpd->time, availabilityStartTime, sizeof(availabilityStartTime));
	dash_format_utc(now, publishTime, sizeof(publishTime));
	
	minimumUpdatePeriod = (unsigned int)MAX(mpd->max_segment_duration / 1000, 1);
	suggestedDelay = minimumUpdatePeriod * 2 + 1;

	if (mpd->flags == DASH_DYNAMIC)
	{
		timeShiftBufferDepth = minimumUpdatePeriod * N_COUNT + 1;
		n = snprintf(playlist, bytes, s_mpd_dynamic, minimumUpdatePeriod, timeShiftBufferDepth, availabilityStartTime,
			suggestedDelay, minimumUpdatePeriod, publishTime);
		n += snprintf(playlist + n, n < bytes ? bytes - n : 0, "  <Period start=\"PT0S\" id=\"dash\">\n");
	}
	else
	{
		n = snprintf(playlist, bytes, s_mpd_static, (unsigned int)(mpd->duration / 1000), minimumUpdatePeriod);
		n += snprintf(playlist + n, n < bytes ? bytes - n : 0, "  <Period start=\"PT0S\" id=\"dash\">\n");
	}

	for (i = 0; i < mpd->count; i++)
	{
		track = &mpd->tracks[i];
		if (dash_mpd_is_video_object(track->object))
		{
			int bw = track->bitrate > 0 ? track->bitrate : 256000;

			n += snprintf(playlist + n, n < bytes ? bytes - n : 0, s_h264, track->name, track->codecs, track->u.video.width, track->u.video.height, track->u.video.frame_rate, bw, track->prefix, track->prefix);
			list_for_each_safe(link, next, &track->root)
			{
				seg = list_entry(link, struct dash_segment_t, link);
				n += snprintf(playlist + n, n < bytes ? bytes - n : 0, "             <S t=\"%" PRId64 "\" d=\"%u\"/>\n", seg->timestamp, (unsigned int)seg->duration);
			}
			n += snprintf(playlist + n, n < bytes ? bytes - n : 0, "%s", s_footer);
		}
		else if (dash_mpd_is_audio_object(track->object))
		{
			int bw = track->bitrate > 0 ? track->bitrate : 64000;
			struct dash_adaptation_set_t* vtrack = dash_mpd_find_video_track(mpd);

			n += snprintf(playlist + n, n < bytes ? bytes - n : 0, s_aac, track->name, track->codecs, track->u.audio.sample_rate, bw, track->u.audio.channel, track->prefix, track->prefix);
			list_for_each_safe(link, next, &track->root)
			{
				seg = list_entry(link, struct dash_segment_t, link);
				/* 仅列出与视频同 $Time$ 的音频，避免播放器只刷 m4a */
				if (vtrack && !dash_mpd_track_has_timestamp(vtrack, seg->timestamp))
					continue;
				n += snprintf(playlist + n, n < bytes ? bytes - n : 0, "             <S t=\"%" PRId64 "\" d=\"%u\"/>\n", seg->timestamp, (unsigned int)seg->duration);
			}
			n += snprintf(playlist + n, n < bytes ? bytes - n : 0, "%s", s_footer);
		}
	}

	n += snprintf(playlist + n, n < bytes ? bytes - n : 0, "  </Period>\n</MPD>\n");
	return n >= bytes ? 0 : n;
}
