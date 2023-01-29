//
// Copyright(C) 2020 Night Dive Studios, Inc.
// Copyright(C) 2023 André Guilherme
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//      Theora Video Streaming
//      Author: Dimitris Giannakis
//
#if defined(LIBTHEORA)
#include "SDL.h"
#include <stdio.h>
#include <string.h>
static Uint32 baseticks = 0;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
//#include <windows.h>
#define THEORAPLAY_THREAD_T    HANDLE
#define THEORAPLAY_MUTEX_T     HANDLE
#define sleepms(x) Sleep(x)
#else
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#define sleepms(x) usleep((x) * 1000)
#define THEORAPLAY_THREAD_T    pthread_t
#define THEORAPLAY_MUTEX_T     pthread_mutex_t
#endif

#include "i_theoraplay.h"
#include "theora/theoradec.h"
#include "vorbis/codec.h"
#include "d_main.h"
#include "i_video.h"
#include "i_system.h"
#include "i_sdlinput.h"
#include "doomtype.h"
#include "doomstat.h"

#ifdef _MSC_VER
#define inline __inline
#endif

// texture to display video
static rbTexture_t          texture;

// global buffers
static uint8_t              *videoBuffer;
static uint8_t              *audioBuffer;

#define THEORAPLAY_INTERNAL 1

typedef THEORAPLAY_VideoFrame VideoFrame;
typedef THEORAPLAY_AudioPacket AudioPacket;
 
typedef unsigned char *(*ConvertVideoFrameFn)(const th_info *tinfo,
	const th_ycbcr_buffer ycbcr);

static unsigned char *ConvertVideoFrame420ToYUVPlanar(
	const th_info *tinfo, const th_ycbcr_buffer ycbcr,
	const int p0, const int p1, const int p2)
{
	int i;
	const int w = tinfo->pic_width;
	const int h = tinfo->pic_height;
	const int yoff = (tinfo->pic_x & ~1) + ycbcr[0].stride * (tinfo->pic_y & ~1);
	const int uvoff = (tinfo->pic_x / 2) + (ycbcr[1].stride) * (tinfo->pic_y / 2);
	unsigned char *yuv = (unsigned char *)malloc(w * h * 2);
	if (yuv)
	{
		unsigned char *dst = yuv;
		for (i = 0; i < h; i++, dst += w)
			memcpy(dst, ycbcr[p0].data + yoff + ycbcr[p0].stride * i, w);
		for (i = 0; i < (h / 2); i++, dst += w / 2)
			memcpy(dst, ycbcr[p1].data + uvoff + ycbcr[p1].stride * i, w / 2);
		for (i = 0; i < (h / 2); i++, dst += w / 2)
			memcpy(dst, ycbcr[p2].data + uvoff + ycbcr[p2].stride * i, w / 2);
	} // if

	return yuv;
} // ConvertVideoFrame420ToYUVPlanar


static unsigned char *ConvertVideoFrame420ToYV12(const th_info *tinfo,
	const th_ycbcr_buffer ycbcr)
{
	return ConvertVideoFrame420ToYUVPlanar(tinfo, ycbcr, 0, 2, 1);
} // ConvertVideoFrame420ToYV12


static unsigned char *ConvertVideoFrame420ToIYUV(const th_info *tinfo,
	const th_ycbcr_buffer ycbcr)
{
	return ConvertVideoFrame420ToYUVPlanar(tinfo, ycbcr, 0, 1, 2);
} // ConvertVideoFrame420ToIYUV


// RGB
#define THEORAPLAY_CVT_FNNAME_420 ConvertVideoFrame420ToRGB
#define THEORAPLAY_CVT_RGB_ALPHA 0
static inline unsigned char *THEORAPLAY_CVT_FNNAME_420(const th_info *tinfo,
	const th_ycbcr_buffer ycbcr)
{
	const int w = tinfo->pic_width;
	const int h = tinfo->pic_height;
	unsigned char *pixels = (unsigned char *)malloc(w * h * 4);
	if (pixels)
	{
		unsigned char *dst = pixels;
		const int ystride = ycbcr[0].stride;
		const int cbstride = ycbcr[1].stride;
		const int crstride = ycbcr[2].stride;
		const int yoff = (tinfo->pic_x & ~1) + ystride * (tinfo->pic_y & ~1);
		const int cboff = (tinfo->pic_x / 2) + (cbstride) * (tinfo->pic_y / 2);
		const unsigned char *py = ycbcr[0].data + yoff;
		const unsigned char *pcb = ycbcr[1].data + cboff;
		const unsigned char *pcr = ycbcr[2].data + cboff;
		int posx, posy;

		for (posy = 0; posy < h; posy++)
		{
			for (posx = 0; posx < w; posx++)
			{
				// http://www.theora.org/doc/Theora.pdf, 1.1 spec,
				//  chapter 4.2 (Y'CbCr -> Y'PbPr -> R'G'B')
				// These constants apparently work for NTSC _and_ PAL/SECAM.
				const float yoffset = 16.0f;
				const float yexcursion = 219.0f;
				const float cboffset = 128.0f;
				const float cbexcursion = 224.0f;
				const float croffset = 128.0f;
				const float crexcursion = 224.0f;
				const float kr = 0.299f;
				const float kb = 0.114f;

				const float y = (((float)py[posx]) - yoffset) / yexcursion;
				const float pb = (((float)pcb[posx / 2]) - cboffset) / cbexcursion;
				const float pr = (((float)pcr[posx / 2]) - croffset) / crexcursion;
				const float r = (y + (2.0f * (1.0f - kr) * pr)) * 255.0f;
				const float g = (y - ((2.0f * (((1.0f - kb) * kb) / ((1.0f - kb) - kr))) * pb) - ((2.0f * (((1.0f - kr) * kr) / ((1.0f - kb) - kr))) * pr)) * 255.0f;
				const float b = (y + (2.0f * (1.0f - kb) * pb)) * 255.0f;

				*(dst++) = (unsigned char)((r < 0.0f) ? 0.0f : (r > 255.0f) ? 255.0f : r);
				*(dst++) = (unsigned char)((g < 0.0f) ? 0.0f : (g > 255.0f) ? 255.0f : g);
				*(dst++) = (unsigned char)((b < 0.0f) ? 0.0f : (b > 255.0f) ? 255.0f : b);
#if THEORAPLAY_CVT_RGB_ALPHA
				*(dst++) = 0xFF;
#endif
			} // for

			// adjust to the start of the next line.
			py += ystride;
			pcb += cbstride * (posy % 2);
			pcr += crstride * (posy % 2);
		} // for
	} // if

	return pixels;
} // THEORAPLAY_CVT_FNNAME_420

#undef THEORAPLAY_CVT_RGB_ALPHA
#undef THEORAPLAY_CVT_FNNAME_420

// RGBA
#define THEORAPLAY_CVT_FNNAME_420 ConvertVideoFrame420ToRGBA
#define THEORAPLAY_CVT_RGB_ALPHA 1
static unsigned char *THEORAPLAY_CVT_FNNAME_420(const th_info *tinfo,
	const th_ycbcr_buffer ycbcr)
{
	const int w = tinfo->pic_width;
	const int h = tinfo->pic_height;
	unsigned char *pixels = (unsigned char *)malloc(w * h * 4);
	if (pixels)
	{
		unsigned char *dst = pixels;
		const int ystride = ycbcr[0].stride;
		const int cbstride = ycbcr[1].stride;
		const int crstride = ycbcr[2].stride;
		const int yoff = (tinfo->pic_x & ~1) + ystride * (tinfo->pic_y & ~1);
		const int cboff = (tinfo->pic_x / 2) + (cbstride) * (tinfo->pic_y / 2);
		const unsigned char *py = ycbcr[0].data + yoff;
		const unsigned char *pcb = ycbcr[1].data + cboff;
		const unsigned char *pcr = ycbcr[2].data + cboff;
		int posx, posy;

		for (posy = 0; posy < h; posy++)
		{
			for (posx = 0; posx < w; posx++)
			{
				// http://www.theora.org/doc/Theora.pdf, 1.1 spec,
				//  chapter 4.2 (Y'CbCr -> Y'PbPr -> R'G'B')
				// These constants apparently work for NTSC _and_ PAL/SECAM.
				const float yoffset = 16.0f;
				const float yexcursion = 219.0f;
				const float cboffset = 128.0f;
				const float cbexcursion = 224.0f;
				const float croffset = 128.0f;
				const float crexcursion = 224.0f;
				const float kr = 0.299f;
				const float kb = 0.114f;

				const float y = (((float)py[posx]) - yoffset) / yexcursion;
				const float pb = (((float)pcb[posx / 2]) - cboffset) / cbexcursion;
				const float pr = (((float)pcr[posx / 2]) - croffset) / crexcursion;
				const float r = (y + (2.0f * (1.0f - kr) * pr)) * 255.0f;
				const float g = (y - ((2.0f * (((1.0f - kb) * kb) / ((1.0f - kb) - kr))) * pb) - ((2.0f * (((1.0f - kr) * kr) / ((1.0f - kb) - kr))) * pr)) * 255.0f;
				const float b = (y + (2.0f * (1.0f - kb) * pb)) * 255.0f;

				*(dst++) = (unsigned char)((r < 0.0f) ? 0.0f : (r > 255.0f) ? 255.0f : r);
				*(dst++) = (unsigned char)((g < 0.0f) ? 0.0f : (g > 255.0f) ? 255.0f : g);
				*(dst++) = (unsigned char)((b < 0.0f) ? 0.0f : (b > 255.0f) ? 255.0f : b);
#if THEORAPLAY_CVT_RGB_ALPHA
				*(dst++) = 0xFF;
#endif
			} // for

			// adjust to the start of the next line.
			py += ystride;
			pcb += cbstride * (posy % 2);
			pcr += crstride * (posy % 2);
		} // for
	} // if

	return pixels;
} // THEORAPLAY_CVT_FNNAME_420

#undef THEORAPLAY_CVT_RGB_ALPHA
#undef THEORAPLAY_CVT_FNNAME_420


typedef struct TheoraDecoder
{
 
	// Thread wrangling...
	int thread_created;
	THEORAPLAY_MUTEX_T lock;
	volatile int halt;
	int thread_done;
	THEORAPLAY_THREAD_T worker;

	// API state...
	THEORAPLAY_Io *io;
	unsigned int maxframes;  // Max video frames to buffer.
	volatile unsigned int prepped;
	volatile unsigned int videocount;  // currently buffered frames.
	volatile unsigned int audioms;  // currently buffered audio samples.
	volatile int hasvideo;
	volatile int hasaudio;
	volatile int decode_error;

	THEORAPLAY_VideoFormat vidfmt;
	ConvertVideoFrameFn vidcvt;

	VideoFrame *videolist;
	VideoFrame *videolisttail;

	AudioPacket *audiolist;
	AudioPacket *audiolisttail;
} TheoraDecoder;


#ifdef _WIN32
static inline int Thread_Create(TheoraDecoder *ctx, void *(*routine) (void*))
{
	ctx->worker = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)routine,
		(LPVOID)ctx,
		0,
		NULL
	);
	return (ctx->worker == NULL);
}
static inline void Thread_Join(THEORAPLAY_THREAD_T thread)
{
	WaitForSingleObject(thread, INFINITE);
	CloseHandle(thread);
}
static inline int Mutex_Create(TheoraDecoder *ctx)
{
	ctx->lock = CreateMutex(NULL, FALSE, NULL);
	return (ctx->lock == NULL);
}
static inline void Mutex_Destroy(THEORAPLAY_MUTEX_T mutex)
{
	CloseHandle(mutex);
}
static inline void Mutex_Lock(THEORAPLAY_MUTEX_T mutex)
{
	WaitForSingleObject(mutex, INFINITE);
}
static inline void Mutex_Unlock(THEORAPLAY_MUTEX_T mutex)
{
	ReleaseMutex(mutex);
}
#endif

static int FeedMoreOggData(THEORAPLAY_Io *io, ogg_sync_state *sync)
{
	long buflen = 4096 * 2;
	char *buffer = ogg_sync_buffer(sync, buflen);
	if (buffer == NULL)
		return -1;

	buflen = io->read(io, buffer, buflen);
	if (buflen <= 0)
		return 0;

	return (ogg_sync_wrote(sync, buflen) == 0) ? 1 : -1;
} // FeedMoreOggData
 
static void WorkerThread(TheoraDecoder *ctx)
{	 
#define queue_ogg_page(ctx) do { \
        if (tpackets) ogg_stream_pagein(&tstream, &page); \
        if (vpackets) ogg_stream_pagein(&vstream, &page); \
    } while (0)

	unsigned long audioframes = 0;
	double fps = 0.0;
	int was_error = 1;  // resets to 0 at the end.
	int eos = 0;  // end of stream flag.

	// Too much Ogg/Vorbis/Theora state...
	ogg_packet packet;
	ogg_sync_state sync;
	ogg_page page;
	int vpackets = 0;
	vorbis_info vinfo;
	vorbis_comment vcomment;
	ogg_stream_state vstream;
	int vdsp_init = 0;
	vorbis_dsp_state vdsp;
	int tpackets = 0;
	th_info tinfo;
	th_comment tcomment;
	ogg_stream_state tstream;
	int vblock_init = 0;
	vorbis_block vblock;
	th_dec_ctx *tdec = NULL;
	th_setup_info *tsetup = NULL;

	ogg_sync_init(&sync);
	vorbis_info_init(&vinfo);
	vorbis_comment_init(&vcomment);
	th_comment_init(&tcomment);
	th_info_init(&tinfo);

	int bos = 1;
	while (!ctx->halt && bos)
	{
		if (FeedMoreOggData(ctx->io, &sync) <= 0)
			goto cleanup;

		// parse out the initial header.
		while ((!ctx->halt) && (ogg_sync_pageout(&sync, &page) > 0))
		{
			ogg_stream_state test;

			if (!ogg_page_bos(&page))  // not a header.
			{
				queue_ogg_page(ctx);
				bos = 0;
				break;
			} // if

			ogg_stream_init(&test, ogg_page_serialno(&page));
			ogg_stream_pagein(&test, &page);
			ogg_stream_packetout(&test, &packet);

			if (!tpackets && (th_decode_headerin(&tinfo, &tcomment, &tsetup, &packet) >= 0))
			{
				memcpy(&tstream, &test, sizeof(test));
				tpackets = 1;
			} // if
			else if (!vpackets && (vorbis_synthesis_headerin(&vinfo, &vcomment, &packet) >= 0))
			{
				memcpy(&vstream, &test, sizeof(test));
				vpackets = 1;
			} // else if
			else
			{
				// whatever it is, we don't care about it
				ogg_stream_clear(&test);
			} // else
		} // while
	} // while

	// no audio OR video?
	if (ctx->halt || (!vpackets && !tpackets))
		goto cleanup;

	// apparently there are two more theora and two more vorbis headers next.
	while ((!ctx->halt) && ((tpackets && (tpackets < 3)) || (vpackets && (vpackets < 3))))
	{
		while (!ctx->halt && tpackets && (tpackets < 3))
		{
			if (ogg_stream_packetout(&tstream, &packet) != 1)
				break; // get more data?
			if (!th_decode_headerin(&tinfo, &tcomment, &tsetup, &packet))
				goto cleanup;
			tpackets++;
		} // while

		while (!ctx->halt && vpackets && (vpackets < 3))
		{
			if (ogg_stream_packetout(&vstream, &packet) != 1)
				break;  // get more data?
			if (vorbis_synthesis_headerin(&vinfo, &vcomment, &packet))
				goto cleanup;
			vpackets++;
		} // while

		// get another page, try again?
		if (ogg_sync_pageout(&sync, &page) > 0)
			queue_ogg_page(ctx);
		else if (FeedMoreOggData(ctx->io, &sync) <= 0)
			goto cleanup;
	} // while

	// okay, now we have our streams, ready to set up decoding.
	if (!ctx->halt && tpackets)
	{
		// th_decode_alloc() docs say to check for insanely large frames yourself.
		if ((tinfo.frame_width > 99999) || (tinfo.frame_height > 99999))
			goto cleanup;

		// We treat "unspecified" as NTSC. *shrug*
		if ((tinfo.colorspace != TH_CS_UNSPECIFIED) &&
			(tinfo.colorspace != TH_CS_ITU_REC_470M) &&
			(tinfo.colorspace != TH_CS_ITU_REC_470BG))
		{
			assert(0 && "Unsupported colorspace.");  // !!! FIXME
			goto cleanup;
		} // if

		if (tinfo.pixel_fmt != TH_PF_420) { assert(0); goto cleanup; } // !!! FIXME

		if (tinfo.fps_denominator != 0)
			fps = ((double)tinfo.fps_numerator) / ((double)tinfo.fps_denominator);

		tdec = th_decode_alloc(&tinfo, tsetup);
		if (!tdec) goto cleanup;

		// Set decoder to maximum post-processing level.
		//  Theoretically we could try dropping this level if we're not keeping up.
		int pp_level_max = 0;
		// !!! FIXME: maybe an API to set this?
		//th_decode_ctl(tdec, TH_DECCTL_GET_PPLEVEL_MAX, &pp_level_max, sizeof(pp_level_max));
		th_decode_ctl(tdec, TH_DECCTL_SET_PPLEVEL, &pp_level_max, sizeof(pp_level_max));
	} // if

	// Done with this now.
	if (tsetup != NULL)
	{
		th_setup_free(tsetup);
		tsetup = NULL;
	} // if

	if (!ctx->halt && vpackets)
	{
		vdsp_init = (vorbis_synthesis_init(&vdsp, &vinfo) == 0);
		if (!vdsp_init)
			goto cleanup;
		vblock_init = (vorbis_block_init(&vdsp, &vblock) == 0);
		if (!vblock_init)
			goto cleanup;
	} // if

	// Now we can start the actual decoding!
	// Note that audio and video don't _HAVE_ to start simultaneously.

	Mutex_Lock(ctx->lock);
	ctx->prepped = 1;
	ctx->hasvideo = (tpackets != 0);
	ctx->hasaudio = (vpackets != 0);
	Mutex_Unlock(ctx->lock);

	while (!ctx->halt && !eos)
	{
		int need_pages = 0;  // need more Ogg pages?
		int saw_video_frame = 0;

		// Try to read as much audio as we can at once. We limit the outer
		//  loop to one video frame and as much audio as we can eat.
		while (!ctx->halt && vpackets)
		{
			float **pcm = NULL;
			const int frames = vorbis_synthesis_pcmout(&vdsp, &pcm);
			if (frames > 0)
			{
				const int channels = vinfo.channels;
				int chanidx, frameidx;
				float *samples;
				AudioPacket *item = (AudioPacket *)malloc(sizeof(AudioPacket));
				if (item == NULL) goto cleanup;
				item->playms = (unsigned long)((((double)audioframes) / ((double)vinfo.rate)) * 1000.0);
				item->channels = channels;
				item->freq = vinfo.rate;
				item->frames = frames;
				item->samples = (float *)malloc(sizeof(float) * frames * channels);
				item->next = NULL;

				if (item->samples == NULL)
				{
					free(item);
					goto cleanup;
				} // if

				// I bet this beats the crap out of the CPU cache...
				samples = item->samples;
				for (frameidx = 0; frameidx < frames; frameidx++)
				{
					for (chanidx = 0; chanidx < channels; chanidx++)
						*(samples++) = pcm[chanidx][frameidx];
				} // for

				vorbis_synthesis_read(&vdsp, frames);  // we ate everything.
				audioframes += frames;

				//printf("Decoded %d frames of audio.\n", (int) frames);
				Mutex_Lock(ctx->lock);
				ctx->audioms += item->playms;
				if (ctx->audiolisttail)
				{
					assert(ctx->audiolist);
					ctx->audiolisttail->next = item;
				} // if
				else
				{
					assert(!ctx->audiolist);
					ctx->audiolist = item;
				} // else
				ctx->audiolisttail = item;
				Mutex_Unlock(ctx->lock);
			} // if

			else  // no audio available left in current packet?
			{
				// try to feed another packet to the Vorbis stream...
				if (ogg_stream_packetout(&vstream, &packet) <= 0)
				{
					if (!tpackets)
						need_pages = 1; // no video, get more pages now.
					break;  // we'll get more pages when the video catches up.
				} // if
				else
				{
					if (vorbis_synthesis(&vblock, &packet) == 0)
						vorbis_synthesis_blockin(&vdsp, &vblock);
				} // else
			} // else
		} // while

		if (!ctx->halt && tpackets)
		{
			// Theora, according to example_player.c, is
			//  "one [packet] in, one [frame] out."
			if (ogg_stream_packetout(&tstream, &packet) <= 0)
				need_pages = 1;
			else
			{
				ogg_int64_t granulepos = 0;

				// you have to guide the Theora decoder to get meaningful timestamps, apparently.  :/
				if (packet.granulepos >= 0)
					th_decode_ctl(tdec, TH_DECCTL_SET_GRANPOS, &packet.granulepos, sizeof(packet.granulepos));

				if (th_decode_packetin(tdec, &packet, &granulepos) == 0)  // new frame!
				{
					th_ycbcr_buffer ycbcr;
					if (th_decode_ycbcr_out(tdec, ycbcr) == 0)
					{
						const double videotime = th_granule_time(tdec, granulepos);
						VideoFrame *item = (VideoFrame *)malloc(sizeof(VideoFrame));
						if (item == NULL) goto cleanup;
						item->playms = (unsigned int)(videotime * 1000.0);
						item->fps = fps;
						item->width = tinfo.pic_width;
						item->height = tinfo.pic_height;
						item->format = ctx->vidfmt;
						item->pixels = ctx->vidcvt(&tinfo, ycbcr);
						item->next = NULL;

						if (item->pixels == NULL)
						{
							free(item);
							goto cleanup;
						} // if

						//printf("Decoded another video frame.\n");
						Mutex_Lock(ctx->lock);
						if (ctx->videolisttail)
						{
							assert(ctx->videolist);
							ctx->videolisttail->next = item;
						} // if
						else
						{
							assert(!ctx->videolist);
							ctx->videolist = item;
						} // else
						ctx->videolisttail = item;
						ctx->videocount++;
						Mutex_Unlock(ctx->lock);

						saw_video_frame = 1;
					} // if
				} // if
			} // else
				} // if

		if (!ctx->halt && need_pages)
		{
			const int rc = FeedMoreOggData(ctx->io, &sync);
			if (rc == 0)
				eos = 1;  // end of stream
			else if (rc < 0)
				goto cleanup;  // i/o error, etc.
			else
			{
				while (!ctx->halt && (ogg_sync_pageout(&sync, &page) > 0))
					queue_ogg_page(ctx);
			} // else
		} // if
 
		// Sleep the process until we have space for more frames.
		if (saw_video_frame)
		{
			int go_on = !ctx->halt;
			//printf("Sleeping.\n");
			while (go_on)
			{
				// !!! FIXME: This is stupid. I should use a semaphore for this.
				Mutex_Lock(ctx->lock);
				go_on = !ctx->halt && (ctx->videocount >= ctx->maxframes);
				Mutex_Unlock(ctx->lock);
				if (go_on)
					sleepms(10);
			} // while
			//printf("Awake!\n");
		} // if
			} // while

	was_error = 0;

cleanup:
	ctx->decode_error = (!ctx->halt && was_error);
	if (tdec != NULL) th_decode_free(tdec);
	if (tsetup != NULL) th_setup_free(tsetup);
	if (vblock_init) vorbis_block_clear(&vblock);
	if (vdsp_init) vorbis_dsp_clear(&vdsp);
	if (tpackets) ogg_stream_clear(&tstream);
	if (vpackets) ogg_stream_clear(&vstream);
	th_info_clear(&tinfo);
	th_comment_clear(&tcomment);
	vorbis_comment_clear(&vcomment);
	vorbis_info_clear(&vinfo);
	ogg_sync_clear(&sync);
	ctx->io->close(ctx->io);
	ctx->thread_done = 1;
		} // WorkerThread


static void *WorkerThreadEntry(void *_this)
{
	TheoraDecoder *ctx = (TheoraDecoder *)_this;
	WorkerThread(ctx);
	//printf("Worker thread is done.\n");
	return NULL;
} // WorkerThreadEntry


static long IoFopenRead(THEORAPLAY_Io *io, void *buf, long buflen)
{
	FILE *f = (FILE *)io->userdata;
	const size_t br = fread(buf, 1, buflen, f);
	if ((br == 0) && ferror(f))
		return -1;
	return (long)br;
} // IoFopenRead


static void IoFopenClose(THEORAPLAY_Io *io)
{
	FILE *f = (FILE *)io->userdata;
	fclose(f);
	free(io);
} // IoFopenClose


THEORAPLAY_Decoder *THEORAPLAY_startDecodeFile(const char *fname,
	const unsigned int maxframes,
	THEORAPLAY_VideoFormat vidfmt)
{
	THEORAPLAY_Io *io = (THEORAPLAY_Io *)malloc(sizeof(THEORAPLAY_Io));
	if (io == NULL)
		return NULL;

	FILE *f = fopen(fname, "rb");
	if (f == NULL)
	{
		free(io);
		return NULL;
	} // if

	io->read = IoFopenRead;
	io->close = IoFopenClose;
	io->userdata = f;
	return THEORAPLAY_startDecode(io, maxframes, vidfmt);
} // THEORAPLAY_startDecodeFile


THEORAPLAY_Decoder *THEORAPLAY_startDecode(THEORAPLAY_Io *io,
	const unsigned int maxframes,
	THEORAPLAY_VideoFormat vidfmt)
{
	TheoraDecoder *ctx = NULL;
	ConvertVideoFrameFn vidcvt = NULL;

	switch (vidfmt)
	{
		// !!! FIXME: current expects TH_PF_420.
#define VIDCVT(t) case THEORAPLAY_VIDFMT_##t: vidcvt = ConvertVideoFrame420To##t; break;
		VIDCVT(YV12)
			VIDCVT(IYUV)
			VIDCVT(RGB)
			VIDCVT(RGBA)
#undef VIDCVT
	default: goto startdecode_failed;  // invalid/unsupported format.
	} // switch

	ctx = (TheoraDecoder *)malloc(sizeof(TheoraDecoder));
	if (ctx == NULL)
		goto startdecode_failed;

	memset(ctx, '\0', sizeof(TheoraDecoder));
	ctx->maxframes = maxframes;
	ctx->vidfmt = vidfmt;
	ctx->vidcvt = vidcvt;
	ctx->io = io;

	if (Mutex_Create(ctx) == 0)
	{
		ctx->thread_created = (Thread_Create(ctx, WorkerThreadEntry) == 0);
		if (ctx->thread_created)
			return (THEORAPLAY_Decoder *)ctx;
	} // if

	Mutex_Destroy(ctx->lock);

startdecode_failed:
	io->close(io);
	free(ctx);
	return NULL;
} // THEORAPLAY_startDecode


void THEORAPLAY_stopDecode(THEORAPLAY_Decoder *decoder)
{
	TheoraDecoder *ctx = (TheoraDecoder *)decoder;
	if (!ctx)
		return;

	if (ctx->thread_created)
	{
		ctx->halt = 1;
		Thread_Join(ctx->worker);
		Mutex_Destroy(ctx->lock);
	} // if

	VideoFrame *videolist = ctx->videolist;
	while (videolist)
	{
		VideoFrame *next = videolist->next;
		free(videolist->pixels);
		free(videolist);
		videolist = next;
	} // while

	AudioPacket *audiolist = ctx->audiolist;
	while (audiolist)
	{
		AudioPacket *next = audiolist->next;
		free(audiolist->samples);
		free(audiolist);
		audiolist = next;
	} // while

	free(ctx);
} // THEORAPLAY_stopDecode


int THEORAPLAY_isDecoding(THEORAPLAY_Decoder *decoder)
{
	TheoraDecoder *ctx = (TheoraDecoder *)decoder;
	int retval = 0;
	if (ctx)
	{
		Mutex_Lock(ctx->lock);
		retval = (ctx && (ctx->audiolist || ctx->videolist ||
			(ctx->thread_created && !ctx->thread_done)));
		Mutex_Unlock(ctx->lock);
	} // if
	return retval;
} // THEORAPLAY_isDecoding


#define GET_SYNCED_VALUE(typ, defval, decoder, member) \
    TheoraDecoder *ctx = (TheoraDecoder *) decoder; \
    typ retval = defval; \
    if (ctx) { \
        Mutex_Lock(ctx->lock); \
        retval = ctx->member; \
        Mutex_Unlock(ctx->lock); \
    } \
    return retval;

int THEORAPLAY_isInitialized(THEORAPLAY_Decoder *decoder)
{
	GET_SYNCED_VALUE(int, 0, decoder, prepped);
} // THEORAPLAY_isInitialized


int THEORAPLAY_hasVideoStream(THEORAPLAY_Decoder *decoder)
{
	GET_SYNCED_VALUE(int, 0, decoder, hasvideo);
} // THEORAPLAY_hasVideoStream


int THEORAPLAY_hasAudioStream(THEORAPLAY_Decoder *decoder)
{
	GET_SYNCED_VALUE(int, 0, decoder, hasaudio);
} // THEORAPLAY_hasAudioStream


unsigned int THEORAPLAY_availableVideo(THEORAPLAY_Decoder *decoder)
{
	GET_SYNCED_VALUE(unsigned int, 0, decoder, videocount);
} // THEORAPLAY_hasAudioStream


unsigned int THEORAPLAY_availableAudio(THEORAPLAY_Decoder *decoder)
{
	GET_SYNCED_VALUE(unsigned int, 0, decoder, audioms);
} // THEORAPLAY_hasAudioStream


int THEORAPLAY_decodingError(THEORAPLAY_Decoder *decoder)
{
	GET_SYNCED_VALUE(int, 0, decoder, decode_error);
} // THEORAPLAY_decodingError


const THEORAPLAY_AudioPacket *THEORAPLAY_getAudio(THEORAPLAY_Decoder *decoder)
{
	TheoraDecoder *ctx = (TheoraDecoder *)decoder;
	AudioPacket *retval;

	Mutex_Lock(ctx->lock);
	retval = ctx->audiolist;
	if (retval)
	{
		ctx->audioms -= retval->playms;
		ctx->audiolist = retval->next;
		retval->next = NULL;
		if (ctx->audiolist == NULL)
			ctx->audiolisttail = NULL;
	} // if
	Mutex_Unlock(ctx->lock);

	return retval;
} // THEORAPLAY_getAudio


void THEORAPLAY_freeAudio(const THEORAPLAY_AudioPacket *_item)
{
	THEORAPLAY_AudioPacket *item = (THEORAPLAY_AudioPacket *)_item;
	if (item != NULL)
	{
		assert(item->next == NULL);
		free(item->samples);
		free(item);
	} // if
} // THEORAPLAY_freeAudio


const THEORAPLAY_VideoFrame *THEORAPLAY_getVideo(THEORAPLAY_Decoder *decoder)
{
	TheoraDecoder *ctx = (TheoraDecoder *)decoder;
	VideoFrame *retval;

	Mutex_Lock(ctx->lock);
	retval = ctx->videolist;
	if (retval)
	{
		ctx->videolist = retval->next;
		retval->next = NULL;
		if (ctx->videolist == NULL)
			ctx->videolisttail = NULL;
		//assert(ctx->videocount > 0);
		ctx->videocount--;
	} // if
	Mutex_Unlock(ctx->lock);

	return retval;
} // THEORAPLAY_getVideo


void THEORAPLAY_freeVideo(const THEORAPLAY_VideoFrame *_item)
{
	THEORAPLAY_VideoFrame *item = (THEORAPLAY_VideoFrame *)_item;
	if (item != NULL)
	{
		assert(item->next == NULL);
		free(item->pixels);
		free(item);
	} // if
} // THEORAPLAY_freeVideo

 

typedef struct AudioQueue {
	const THEORAPLAY_AudioPacket *audio;
	int offset;
	struct AudioQueue *next;
} AudioQueue;

static volatile AudioQueue *audio_queue = NULL;
static volatile AudioQueue *audio_queue_tail = NULL;

static void SDLCALL audio_callback(void *userdata, Uint8 *stream, int len) {
	Sint16 *dst = (Sint16 *)stream;
	int sfxVolume = 8;
	while (audio_queue && (len > 0)) {
		volatile AudioQueue *item = audio_queue;
		AudioQueue *next = item->next;
		const int channels = item->audio->channels;

		const float *src = item->audio->samples + (item->offset * channels);
		int cpy = (item->audio->frames - item->offset) * channels;
		int i;

		if (cpy > (len / sizeof(Sint16)))
			cpy = len / sizeof(Sint16);

		for (i = 0; i < cpy; i++) {
			const float val = *(src++);

			if (val < -1.0f)
				*(dst++) = -32768 * ((float)sfxVolume / 15.0f);
			else if (val > 1.0f)
				*(dst++) = 32767 * ((float)sfxVolume / 15.0f);
			else
				*(dst++) = (Sint16)(val * 32767.0f) * ((float)sfxVolume / 15.0f);
		}

		item->offset += (cpy / channels);
		len -= cpy * sizeof(Sint16);

		if (item->offset >= item->audio->frames) {
			THEORAPLAY_freeAudio(item->audio);
			SDL_free((void *)item);
			audio_queue = next;
		}
	}

	if (!audio_queue)
		audio_queue_tail = NULL;

	if (len > 0)
		memset(dst, '\0', len);
}


static void queue_audio(const THEORAPLAY_AudioPacket *audio) {
	AudioQueue *item = (AudioQueue *)SDL_malloc(sizeof(AudioQueue));
	if (!item) {
		THEORAPLAY_freeAudio(audio);
		return;
	}

	item->audio = audio;
	item->offset = 0;
	item->next = NULL;

	SDL_LockAudio();
	if (audio_queue_tail)
		audio_queue_tail->next = item;
	else
		audio_queue = item;
	audio_queue_tail = item;
	SDL_UnlockAudio();
}


void I_AVStartVideoStream(const char *fname) 
{
 
	THEORAPLAY_Decoder *decoder = NULL;
	const THEORAPLAY_VideoFrame *video = NULL;
	const THEORAPLAY_AudioPacket *audio = NULL;
	SDL_Window *screen = NULL; 
	 
	SDL_AudioSpec spec;
    SDL_AudioSpec specobt;
	SDL_Event event;
	Uint32 framems = 0;
	int initfailed = 0;
	int quit = 0;

	float tx;
	int i;	 
	int wi, hi, ws, hs;
	float ri, rs;
	int scalew, scaleh;
	int xoffs = 0, yoffs = 0;
 
	decoder = THEORAPLAY_startDecodeFile(fname, 30, THEORAPLAY_VIDFMT_RGB);
	if (decoder == NULL) {
		printf("I_AVStartVideoStream: Could not decode %s, skipping...\n", fname);
		return;
	}

	while (!audio || !video) {
		if (!audio) audio = THEORAPLAY_getAudio(decoder);
		if (!video) video = THEORAPLAY_getVideo(decoder);
		SDL_Delay(10);
	}

	int width = video->width;
	int height = video->height;

	framems = (video->fps == 0.0) ? 0 : ((Uint32)(1000.0 / video->fps));	 
	videoBuffer = (uint8_t *)malloc(video->width * video->height * 4);
	initfailed = quit = (!videoBuffer || !window);

	// setup texture
	texture.colorMode = TCR_RGB;
	texture.origwidth = video->width;
	texture.origheight = video->height;
	texture.width = texture.origwidth;
	texture.height = texture.origheight;

	RB_UploadTexture(&texture, videoBuffer, TC_CLAMP, TF_LINEAR);
	RB_BindTexture(&texture);
	memset(&spec, '\0', sizeof(SDL_AudioSpec));
	spec.freq = audio->freq;
	spec.format = AUDIO_S16SYS;
	spec.channels = audio->channels;
	spec.samples = 2048;
	spec.callback = audio_callback;
	initfailed = quit = (initfailed || (SDL_OpenAudio(&spec, NULL) != 0));

	void *pixels = NULL;
	int pitch = 0;

	while (audio) {
		queue_audio(audio);
		audio = THEORAPLAY_getAudio(decoder);
	}

	baseticks = SDL_GetTicks();

	if (!quit)
		SDL_PauseAudio(0);

	while (!quit && THEORAPLAY_isDecoding(decoder)) {
		const Uint32 now = SDL_GetTicks() - baseticks;

		if (!video)
			video = THEORAPLAY_getVideo(decoder);

		if (video && (video->playms <= now)) {
			if (framems && ((now - video->playms) >= framems)) 
			{
				const THEORAPLAY_VideoFrame *last = video;
				while ((video = THEORAPLAY_getVideo(decoder)) != NULL) {
					THEORAPLAY_freeVideo(last);
					last = video;
					if ((now - video->playms) < framems)
						break;
				}

				if (!video)
					video = last;
			}
	 
			memcpy(videoBuffer, video->pixels, video->width * video->height * 3);

			
			RB_UpdateTexture(&texture, videoBuffer);

			THEORAPLAY_freeVideo(video);

			video = NULL;
		}
		else {
			SDL_Delay(10);
		}

		while ((audio = THEORAPLAY_getAudio(decoder)) != NULL)
			queue_audio(audio);


		while (window && SDL_PollEvent(&event)) {
			switch (event.type) {

            case SDL_APP_TERMINATING:
                // A platform may be waiting on the app to respond to the quit signal, perform this now so we don't hold it up.
                //I_DoPlatformQuit();
			case SDL_QUIT:
				quit = 1;
				break;

            case SDL_KEYDOWN:
			case SDL_JOYBUTTONDOWN:				// any button
				quit = 1;
				break;
			}
		}

		I_UpdateGrab();
        event_t* ev;
        while ((ev = D_PostEvent) != NULL)
        {
            if (ev->type == ev->type == ev_keydown)
            {
                quit = 1;
            }
        }
	 
		glClearColor(0, 0, 0, 1);
		RB_ClearBuffer(GLCB_COLOR);
 
		vtx_t v[4];
		int texwidth;
		int texheight;

		v[0].tu = v[2].tu = 0;
		v[0].tv = v[1].tv = 0;
		v[1].tu = v[3].tu = 1;
		v[2].tv = v[3].tv = 1;

		RB_SetVertexColor(v, 0xff, 0xff, 0xff, 0xff, 4);
		v[0].z = v[1].z = v[2].z = v[3].z = 0;
 
		SDL_GetWindowSize(window, &ws, &hs);

        glPushAttrib(GL_VIEWPORT_BIT);
        glViewport(0, 0, ws, hs);
 
		RB_SetMaxOrtho(ws, hs);

        glPopAttrib();

		texwidth =  texture.width;
		texheight =  texture.height;

		tx = (float)texture.origwidth / (float)texture.width;

		for (i = 0; i < 4; ++i)
		{
			v[i].r = v[i].g = v[i].b = v[i].a = 0xff;
			v[i].z = 0;
		}

		wi = width;
		hi = height;

		rs = (float)ws / hs;
		ri = (float)wi / hi;

		if (rs > ri)
		{
			scalew = wi * hs / hi;
			scaleh = hs;
		}
		else
		{
			scalew = ws;
			scaleh = hi * ws / wi;
		}

		if (scalew < ws)
		{
			xoffs = (ws - scalew) / 2;
		}
		if (scaleh < hs)
		{
			yoffs = (hs - scaleh) / 2;
		}

		v[0].x = v[2].x = xoffs;
		v[0].y = v[1].y = yoffs;
		v[1].x = v[3].x = xoffs + scalew;
		v[2].y = v[3].y = yoffs + scaleh;

		v[0].tu = v[2].tu = 0;
		v[0].tv = v[1].tv = 0;
		v[1].tu = v[3].tu = tx;
		v[2].tv = v[3].tv = 1;

		RB_ChangeTexParameters(&texture, TC_REPEAT, TF_LINEAR);
		RB_BindTexture(&texture);
	 
		RB_SetBlend(GLSRC_SRC_ALPHA, GLDST_ONE_MINUS_SRC_ALPHA);
		RB_SetState(GLSTATE_CULL, true);
		RB_SetCull(GLCULL_FRONT);
		RB_SetState(GLSTATE_DEPTHTEST, false);
		RB_SetState(GLSTATE_BLEND, true);
		RB_SetState(GLSTATE_ALPHATEST, false);

		// render
		RB_DrawVtxQuadImmediate(v);		
		glFinish();
		RB_SwapBuffers();
	}

    int tourchAudio = 0;
	while (!tourchAudio) {
		SDL_LockAudio();
        tourchAudio = (audio_queue == NULL);
        if (!tourchAudio)
        {
            while (audio_queue)
            {
                volatile AudioQueue *item = audio_queue;
                AudioQueue *next = item->next;
                {
                    THEORAPLAY_freeAudio(item->audio);
                    SDL_free((void *)item);
                    audio_queue = next;
                }
            }

            if (!audio_queue)
                audio_queue_tail = NULL;
        }
		SDL_UnlockAudio();

        SDL_Delay(100);
	}

	if (initfailed)
		printf("Initialization failed!\n");
	else if (THEORAPLAY_decodingError(decoder))
		printf("There was an error decoding this file!\n");
	else
		printf("done with this file!\n");

	if (video) THEORAPLAY_freeVideo(video);
	if (audio) THEORAPLAY_freeAudio(audio);
	if (decoder) THEORAPLAY_stopDecode(decoder);
	SDL_CloseAudio();
 
}

//
// RB_SetBlend
//

void RB_SetBlend(int src, int dest)
{
	int pBlend = (rbState.blendSrc ^ src) | (rbState.blendDest ^ dest);
	int glSrc = GL_ONE;
	int glDst = GL_ONE;

	if (pBlend == 0)
		return; // already set

	switch (src)
	{
	case GLSRC_ZERO:
		glSrc = GL_ZERO;
		break;

	case GLSRC_ONE:
		glSrc = GL_ONE;
		break;

	case GLSRC_DST_COLOR:
		glSrc = GL_DST_COLOR;
		break;

	case GLSRC_ONE_MINUS_DST_COLOR:
		glSrc = GL_ONE_MINUS_DST_COLOR;
		break;

	case GLSRC_SRC_ALPHA:
		glSrc = GL_SRC_ALPHA;
		break;

	case GLSRC_ONE_MINUS_SRC_ALPHA:
		glSrc = GL_ONE_MINUS_SRC_ALPHA;
		break;

	case GLSRC_DST_ALPHA:
		glSrc = GL_DST_ALPHA;
		break;

	case GLSRC_ONE_MINUS_DST_ALPHA:
		glSrc = GL_ONE_MINUS_DST_ALPHA;
		break;

	case GLSRC_ALPHA_SATURATE:
		glSrc = GL_SRC_ALPHA_SATURATE;
		break;
	}

	switch (dest) {
	case GLDST_ZERO:
		glDst = GL_ZERO;
		break;

	case GLDST_ONE:
		glDst = GL_ONE;
		break;

	case GLDST_SRC_COLOR:
		glDst = GL_SRC_COLOR;
		break;

	case GLDST_ONE_MINUS_SRC_COLOR:
		glDst = GL_ONE_MINUS_SRC_COLOR;
		break;

	case GLDST_SRC_ALPHA:
		glDst = GL_SRC_ALPHA;
		break;

	case GLDST_ONE_MINUS_SRC_ALPHA:
		glDst = GL_ONE_MINUS_SRC_ALPHA;
		break;

	case GLDST_DST_ALPHA:
		glDst = GL_DST_ALPHA;
		break;

	case GLDST_ONE_MINUS_DST_ALPHA:
		glDst = GL_ONE_MINUS_DST_ALPHA;
		break;
	}

	glBlendFunc(glSrc, glDst);

	rbState.blendSrc = src;
	rbState.blendDest = dest;
	rbState.numStateChanges++;
}

#endif