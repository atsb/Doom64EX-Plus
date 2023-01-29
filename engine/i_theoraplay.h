//
// Copyright(C) 2014 Night Dive Studios, Inc.
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


#ifndef __I__THEORAPLAY_H__
#define __I__THEORAPLAY_H__
 
#ifdef __cplusplus
extern "C" {
#endif

	void I_AVStartVideoStream(const char *filename);

	typedef struct THEORAPLAY_Io THEORAPLAY_Io;
	struct THEORAPLAY_Io
	{
		long(*read)(THEORAPLAY_Io *io, void *buf, long buflen);
		void(*close)(THEORAPLAY_Io *io);
		void *userdata;
	};

	typedef struct THEORAPLAY_Decoder THEORAPLAY_Decoder;

	/* YV12 is YCrCb, not YCbCr; that's what SDL uses for YV12 overlays. */
	typedef enum THEORAPLAY_VideoFormat
	{
		THEORAPLAY_VIDFMT_YV12,  /* NTSC colorspace, planar YCrCb 4:2:0 */
		THEORAPLAY_VIDFMT_IYUV,  /* NTSC colorspace, planar YCbCr 4:2:0 */
		THEORAPLAY_VIDFMT_RGB,   /* 24 bits packed pixel RGB */
		THEORAPLAY_VIDFMT_RGBA   /* 32 bits packed pixel RGBA (full alpha). */
	} THEORAPLAY_VideoFormat;

	typedef struct THEORAPLAY_VideoFrame
	{
		unsigned int playms;
		double fps;
		unsigned int width;
		unsigned int height;
		THEORAPLAY_VideoFormat format;
		unsigned char *pixels;
		struct THEORAPLAY_VideoFrame *next;
	} THEORAPLAY_VideoFrame;

	typedef struct THEORAPLAY_AudioPacket
	{
		unsigned int playms;  /* playback start time in milliseconds. */
		int channels;
		int freq;
		int frames;
		float *samples;  /* frames * channels float32 samples. */
		struct THEORAPLAY_AudioPacket *next;
	} THEORAPLAY_AudioPacket;

	THEORAPLAY_Decoder *THEORAPLAY_startDecodeFile(const char *fname,
		const unsigned int maxframes,
		THEORAPLAY_VideoFormat vidfmt);
	THEORAPLAY_Decoder *THEORAPLAY_startDecode(THEORAPLAY_Io *io,
		const unsigned int maxframes,
		THEORAPLAY_VideoFormat vidfmt);
	void THEORAPLAY_stopDecode(THEORAPLAY_Decoder *decoder);

	int THEORAPLAY_isDecoding(THEORAPLAY_Decoder *decoder);
	int THEORAPLAY_decodingError(THEORAPLAY_Decoder *decoder);
	int THEORAPLAY_isInitialized(THEORAPLAY_Decoder *decoder);
	int THEORAPLAY_hasVideoStream(THEORAPLAY_Decoder *decoder);
	int THEORAPLAY_hasAudioStream(THEORAPLAY_Decoder *decoder);
	unsigned int THEORAPLAY_availableVideo(THEORAPLAY_Decoder *decoder);
	unsigned int THEORAPLAY_availableAudio(THEORAPLAY_Decoder *decoder);

	const THEORAPLAY_AudioPacket *THEORAPLAY_getAudio(THEORAPLAY_Decoder *decoder);
	void THEORAPLAY_freeAudio(const THEORAPLAY_AudioPacket *item);

	const THEORAPLAY_VideoFrame *THEORAPLAY_getVideo(THEORAPLAY_Decoder *decoder);
	void THEORAPLAY_freeVideo(const THEORAPLAY_VideoFrame *item);

 
#ifdef __cplusplus
}
#endif


#endif

