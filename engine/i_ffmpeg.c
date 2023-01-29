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
// DESCRIPTION:
//      FFMpeg Video Streaming
//      Author: Samuel Villarreal
//

#if defined(NIGHTDIVE)

#include <SDL.h>

#include "i_ffmpeg.h"
#include "i_video.h"
#include "i_system.h"
#include "i_sdlinput.h"
#include "doomtype.h"

#ifdef _MSC_VER
    #define inline __inline
#endif

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/time.h"
#include <libavutil/imgutils.h>
//
// because apparently, we still need to use deprecated functions....
//
#if defined(__ICL) || defined (__INTEL_COMPILER)
    #define FF_DISABLE_DEPRECATION_WARNINGS __pragma(warning(push)) __pragma(warning(disable:1478))
    #define FF_ENABLE_DEPRECATION_WARNINGS  __pragma(warning(pop))
#elif defined(_MSC_VER)
    #define FF_DISABLE_DEPRECATION_WARNINGS __pragma(warning(push)) __pragma(warning(disable:4996))
    #define FF_ENABLE_DEPRECATION_WARNINGS  __pragma(warning(pop))
#else
    #define FF_DISABLE_DEPRECATION_WARNINGS _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
    #define FF_ENABLE_DEPRECATION_WARNINGS  _Pragma("GCC diagnostic warning \"-Wdeprecated-declarations\"")
#endif

//
// the buffer size passed into the post mix callback routine can vary, depending
// on the OS. I am expecting at least 4k tops but if needed, it can be bumped to 8k
//
#define SDL_AUDIO_BUFFER_SIZE       4096
#define MAX_AUDIO_FRAME_SIZE        192000

//=============================================================================
//
// Structs
//
//=============================================================================

//
// every video and audio packet that we recieve from the decoder is thrown into a queue
// which is all done by the thread routine. the drawer and post mix functions are
// responsible from accessing each entry in the queue
//

typedef struct avPacketData_s
{
    AVPacket                    *packet;
    struct avPacketData_s       *next;
    boolean                     endMark;
} avQueuePacketData_t;

typedef struct
{
    avQueuePacketData_t         *first;
    SDL_mutex                   *mutex;
} avPacketQueue_t;

typedef struct avAudioQueueData_s
{
    uint8_t                     buffer[SDL_AUDIO_BUFFER_SIZE];
    int                         size;
    int                         offset;
    double                      timestamp;
    struct avAudioQueueData_s   *next;
} avAudioQueueData_t;

typedef struct
{
    avAudioQueueData_t          *first;
    SDL_mutex                   *mutex;
} avAudioQueue_t;

//=============================================================================
//
// Locals
//
//=============================================================================

// ffmpeg contexts
static struct SwsContext    *swsCtx;
static AVFormatContext      *formatCtx;
static AVCodecContext       *videoCodecCtx;
static AVCodecContext       *audioCodecCtx;

// codecs
static AVCodec              *videoCodec;
static AVCodec              *audioCodec;

// global video frame
static AVFrame              *videoFrame;

// global buffers
static uint8_t              *videoBuffer;
static uint8_t              *audioBuffer;

// global timestamps
static int64_t              currentPts;
static uint64_t             globalPts;
static double               audioPts;

// dimentions
static int                  reqWidth;
static int                  reqHeight;

// stream indexes
static int                  videoStreamIdx;
static int                  audioStreamIdx;

// main thread
static SDL_Thread           *thread;
// texture to display video
static rbTexture_t          texture;
// user pressed a button
static boolean              userExit;

// is audio enabled?
static boolean              hasAudio;

// all video frames processed
static boolean              videoFinished;

// all audio frames processed
static boolean              audioFinished;

// keep track of holding down the key
static boolean              userPressed;

// packet queues
static avPacketQueue_t      *videoPacketQueue;
static avPacketQueue_t      *audioPacketQueue;

// audio buffer queue
static avAudioQueue_t       audioQueue;

// clock speed
static double               videoClock;
static double               audioClock;

// length of the frame in time
static double               frameTime;
static double               lastFrameTime;

//=============================================================================
//
// Packet Querying
//
//=============================================================================

//
// I_AVAllocQueuePacketData
//

static avQueuePacketData_t *I_AVAllocQueuePacketData(AVPacket *packet)
{
    avQueuePacketData_t *packetItem = (avQueuePacketData_t*)malloc(sizeof(avQueuePacketData_t));

    if(packet)
    {
        packetItem->packet = (AVPacket*)malloc(sizeof(AVPacket));
        memcpy(packetItem->packet, packet, sizeof(AVPacket));
        packetItem->endMark = false;
    }
    else
    {
        packetItem->packet = NULL;
        packetItem->endMark = true;
    }

    packetItem->next = NULL;
    
    return packetItem;
}

//
// I_AVAllocQueuePacket
//

static avPacketQueue_t *I_AVAllocQueuePacket(void)
{
    avPacketQueue_t *packetQueue = (avPacketQueue_t*)malloc(sizeof(avPacketQueue_t));
    packetQueue->first = NULL;
    
    packetQueue->mutex = SDL_CreateMutex();
    return packetQueue;
}

//
// I_AVPushPacketToQueue
//
// Adds a video packet the queue list
// Called from SDL thread
//

static void I_AVPushPacketToQueue(avPacketQueue_t *packetQueue, AVPacket *packet)
{
    avQueuePacketData_t **ptr;
    
    SDL_LockMutex(packetQueue->mutex);
    ptr = &packetQueue->first;
    
    while(*ptr)
    {
        ptr = &(*ptr)->next;
    }
    
    *ptr = I_AVAllocQueuePacketData(packet);
    
    SDL_UnlockMutex(packetQueue->mutex);
}

//
// I_AVPopPacketFromQueue
//

static boolean I_AVPopPacketFromQueue(avPacketQueue_t *packetQueue, AVPacket **packet)
{
    boolean result = true;

    *packet = NULL;

    if(packetQueue->mutex)
    {
        SDL_LockMutex(packetQueue->mutex);
    }

    if(packetQueue->first)
    {
        if(!packetQueue->first->endMark)
        {
            void *del;
            
            *packet = packetQueue->first->packet;
            del = packetQueue->first;
            packetQueue->first = packetQueue->first->next;
            
            free(del);
        }
        else
        {
            // needed to indicate that we reached the end
            result = false;
        }
    }
    
    if(packetQueue->mutex)
    {
        SDL_UnlockMutex(packetQueue->mutex);
    }

    return result;
}

//
// I_AVDeletePacketQueue
//

static void I_AVDeletePacketQueue(avPacketQueue_t **packetQueue)
{
    SDL_DestroyMutex((*packetQueue)->mutex);
    (*packetQueue)->mutex = NULL;

    while(1)
    {
        AVPacket *packet;

        if(!I_AVPopPacketFromQueue(*packetQueue, &packet) || packet == NULL)
        {
            break;
        }
        
        av_packet_unref(packet);
    }

    free(*packetQueue);
    *packetQueue = NULL;
}

//=============================================================================
//
// Audio Buffer Querying
//
//=============================================================================

//
// I_AVAllocAudioQueueData
//

static avAudioQueueData_t *I_AVAllocAudioQueueData(uint8_t *buffer, const int size)
{
    avAudioQueueData_t *audioData = (avAudioQueueData_t*)malloc(sizeof(avAudioQueueData_t));

    audioData->size = size;
    audioData->next = NULL;
    audioData->offset = 0;
    audioData->timestamp = audioPts;

    memcpy(audioData->buffer, buffer, size);
    return audioData;
}

//
// I_AVPushAudioToQueue
//
// Adds a fixed amount of audio buffer to the queue.
// If the size is less than the max buffer size specified
// by SDL_AUDIO_BUFFER_SIZE, then the current buffer in
// the queue will be reused the next time this function
// is called until it's full
//

static void I_AVPushAudioToQueue(uint8_t *buffer, const int size)
{
    avAudioQueueData_t **ptr;
    int bufsize = size;
    uint8_t *buf = buffer;
    
    SDL_LockMutex(audioQueue.mutex);
    ptr = &audioQueue.first;
    
    while(bufsize > 0)
    {
        // we'll need to create a new audio queue here
        if(*ptr == NULL)
        {
            // exceeded the buffer limit?
            if(bufsize > SDL_AUDIO_BUFFER_SIZE)
            {
                int remaining = (bufsize - SDL_AUDIO_BUFFER_SIZE);

                // copy what we have
                *ptr = I_AVAllocAudioQueueData(buf, SDL_AUDIO_BUFFER_SIZE);
                bufsize = remaining;
                buf += SDL_AUDIO_BUFFER_SIZE;
            }
            else
            {
                *ptr = I_AVAllocAudioQueueData(buf, bufsize);
                // we don't need to do anything else
                break;
            }
        }

        // buffer not filled?
        if((*ptr)->size < SDL_AUDIO_BUFFER_SIZE)
        {
            int len = bufsize + (*ptr)->size;

            // exceeded the buffer limit?
            if(len > SDL_AUDIO_BUFFER_SIZE)
            {
                int remaining = (SDL_AUDIO_BUFFER_SIZE - (*ptr)->size);

                // copy what we have
                memcpy((*ptr)->buffer + (*ptr)->size, buf, remaining);
                (*ptr)->size = SDL_AUDIO_BUFFER_SIZE;

                bufsize -= remaining;
                buf += remaining;
            }
            else
            {
                // copy into the existing buffer
                memcpy((*ptr)->buffer + (*ptr)->size, buf, bufsize);
                (*ptr)->size += bufsize;

                // we don't need to do anything else
                SDL_UnlockMutex(audioQueue.mutex);
                return;
            }
        }

        ptr = &(*ptr)->next;
    }
    
    SDL_UnlockMutex(audioQueue.mutex);
}

//
// I_AVPopAudioFromQueue
//
// Retrieves an audio buffer from the queue and gets the
// timestamp that it's based on. The buffer in the queue
// won't be freed until the entire buffer has been read
// which 'filled' will be set to true
//
// Called from the Post-Mix callback routine
//

static uint8_t *I_AVPopAudioFromQueue(const int size, double *timestamp, boolean *filled)
{
    static uint8_t buffer[SDL_AUDIO_BUFFER_SIZE];
    uint8_t *result = NULL;
    
    if(filled)
    {
        *filled = false;
    }
    
    if(timestamp)
    {
        *timestamp = 0;
    }

    if(audioQueue.mutex)
    {
        SDL_LockMutex(audioQueue.mutex);
    }

    if(audioQueue.first)
    {
        void *del;

        memcpy(buffer, audioQueue.first->buffer + audioQueue.first->offset, size);
        audioQueue.first->offset += size;
        result = buffer;

        if(timestamp)
        {
            *timestamp = audioQueue.first->timestamp;
        }

        // if the entire buffer was read, then we need to free this queue
        if(audioQueue.first->offset >= audioQueue.first->size)
        {
            del = audioQueue.first;
            audioQueue.first = audioQueue.first->next;
        
            free(del);
            
            if(filled)
            {
                *filled = true;
            }
        }
    }
    
    if(audioQueue.mutex)
    {
        SDL_UnlockMutex(audioQueue.mutex);
    }

    return result;
}

//
// I_AVDeleteAudioQueue
//

static void I_AVDeleteAudioQueue(void)
{
    SDL_DestroyMutex(audioQueue.mutex);
    audioQueue.mutex = NULL;

    while(I_AVPopAudioFromQueue(SDL_AUDIO_BUFFER_SIZE, NULL, NULL) != NULL);
}

//=============================================================================
//
// Video/Audio Clock Comparison
//
//=============================================================================

//
// I_AVVideoClockBehind
//

static boolean I_AVVideoClockBehind(void)
{
    return (hasAudio && (videoClock < audioClock));
}

//=============================================================================
//
// Audio Functions
//
//=============================================================================

//
// I_AVFillAudioBuffer
//
// Decodes an audio sample from the AVPacket pointer and then
// pushes it into the queue list for the post mix callback function
// to process.
//
// Called from SDL thread
//

static void I_AVFillAudioBuffer(AVPacket *packet)
{
    static AVFrame frame;
    int length;
    int dataSize;
    int lineSize;
    int frameDone = 0;
    uint16_t *samples;
    int n;
    int size;

    length = avcodec_receive_frame(audioCodecCtx, &frame);
    dataSize = av_samples_get_buffer_size(&lineSize,
                                          audioCodecCtx->channels,
                                          frame.nb_samples,
                                          audioCodecCtx->sample_fmt,
                                          1);

    if(dataSize <= 0)
    {
        // no data yet, need more frames
        return;
    }

    if(packet->pts != AV_NOPTS_VALUE)
    {
        audioPts = av_q2d(audioCodecCtx->time_base) * packet->pts;
    }

    n = 2 * audioCodecCtx->channels;
    audioPts += (double)dataSize / (double)(n * audioCodecCtx->sample_rate);

    samples = (uint16_t*)audioBuffer;

    if(frameDone)
    {
        int ls = lineSize / sizeof(float);
        int write_p = 0;
        int nb, ch;

        // get the audio samples
        // WARNING: this only applies to AV_SAMPLE_FMT_FLTP
        for(nb = 0; nb < ls; nb++)
        {
            for(ch = 0; ch < audioCodecCtx->channels; ch++)
            {
                samples[write_p] = ((float*)frame.extended_data[ch])[nb] * 32767;
                write_p++;
            }
        }

        size = (ls) * sizeof(uint16_t) * audioCodecCtx->channels;

        // add the buffer (partial or not) to the queue
        I_AVPushAudioToQueue((uint8_t*)samples, size);
    }
}

//
// I_AVPostMixCallback
//
// Called frequently by SDL_Mixer
//

static void I_AVPostMixCallback(void *udata, uint8_t *stream, int len)
{
    uint8_t *buf;
    double timestamp;
    extern int sfxVolume;
    boolean filled;
    
    if(!audioPacketQueue || userExit)
    {
        return;
    }

    if((buf = I_AVPopAudioFromQueue(len, &timestamp, &filled)))
    {
        int i;
        Sint16 *samples = (Sint16*)buf;

        // adjust volume
        for(i = 0; i < len/2; i++)
        {
            float scaled = (float)samples[i] * ((float)sfxVolume / 15.0f);

            if(scaled < -32768.0f)
            {
                scaled = -32768.0f;
            }
            if(scaled > 32767.0f)
            {
                scaled = 32767.0f;
            }

            samples[i] = (Sint16)scaled;
        }

        memcpy(stream, buf, len);
        audioFinished = false;

        // update the audio clock ONLY after the buffer in the queue
        // has been fully read
        if(filled)
        {
            // get the current audio clock time
            audioClock = timestamp;
        }
    }
    else
    {
        audioFinished = true;
    }
}

//=============================================================================
//
// Video Functions
//
//=============================================================================

//
// I_AVYUVToBuffer
//

static void I_AVYUVToBuffer(AVFrame *frame)
{
    int y, u, v;
    int r, g, b;
    int px, py;
    int i;
    byte *yptr = frame->data[0];
    byte *uptr = frame->data[1];
    byte *vptr = frame->data[2];
    
    i = 0;
    
    for(py = 0; py < reqHeight; py++)
    {
        for(px = 0; px < reqWidth; px++, i += 3)
        {
            y = yptr[py     * frame->linesize[0] + px    ];
            u = uptr[py / 2 * frame->linesize[1] + px / 2];
            v = vptr[py / 2 * frame->linesize[2] + px / 2];
            
            r = y + 1.402f   * (v - 128);
            g = y - 0.34414f * (u - 128) - 0.71414f * (v - 128);
            b = y + 1.772f   * (u - 128);
            
            videoBuffer[i    ] = max(min(r, 255), 0);
            videoBuffer[i + 1] = max(min(g, 255), 0);
            videoBuffer[i + 2] = max(min(b, 255), 0);
        }
    }
}

//
// I_AVUpdateVideoClock
//
// Try to sync the video clock with the given timestamp
// gathered from the frame packet
//

static double I_AVUpdateVideoClock(AVFrame *frame, double pts)
{
    double frameDelay;

    if(pts != 0)
    {
        videoClock = pts;
    }
    else
    {
        pts = videoClock;
    }

    frameDelay = av_q2d(videoCodecCtx->time_base);
    frameDelay += frame->repeat_pict * (frameDelay * 0.5);

    videoClock += frameDelay;
    return pts;
}

//
// I_AVProcessNextVideoFrame
//

static void I_AVProcessNextVideoFrame(void)
{
    int frameDone = 0;
    AVFrame *frame;
    AVPacket *packet;
    double pts = 0;
    boolean behind = false;

    if(hasAudio)
    {
        if(videoClock > audioClock || audioClock <= 0)
        {
            // don't process if the audio clock hasn't started
            // or if the video clock is ahead though
            // this shouldn't be needed but just in case....
            return;
        }
    }
    
    frame = av_frame_alloc();

    if(hasAudio)
    {
        behind = audioFinished ? true : I_AVVideoClockBehind();
    }

    // collect packets until we have a frame
    while(!frameDone || behind)
    {
        if(!I_AVPopPacketFromQueue(videoPacketQueue, &packet))
        {
            videoFinished = true;
            break;
        }
        
        if(packet == NULL)
        {
            break;
        }

        // get presentation timestamp
        pts = 0;
        globalPts = packet->pts;
        
        avcodec_receive_frame(videoCodecCtx, frame);

        // get the decompression timestamp from this packet
        if(packet->dts == AV_NOPTS_VALUE && frame->opaque &&
            *(uint64_t*)frame->opaque != AV_NOPTS_VALUE)
        {
            pts = *(uint64_t*)frame->opaque;
        }
        else if(packet->dts != AV_NOPTS_VALUE)
        {
            pts = packet->dts;
        }
        else
        {
            pts = 0;
        }

        // approximate the timestamp
        pts *= av_q2d(videoCodecCtx->time_base);

        if(frameDone)
        {
            // update the video clock and frame time
            pts = I_AVUpdateVideoClock(frame, pts);
            frameTime = (pts - lastFrameTime) * 1000.0;
            lastFrameTime = pts;
            currentPts = av_gettime();
            
            if(hasAudio)
            {
                // need to keep processing if we're behind
                // some frames may be skipped
                behind = I_AVVideoClockBehind();
            }
        }

        av_packet_unref(packet);
    }
    
    if(frameDone)
    {
        // convert the decoded data to color data
        sws_scale(swsCtx,
                  (uint8_t const*const*)frame->data,
                  frame->linesize,
                  0,
                  videoCodecCtx->height,
                  videoFrame->data,
                  videoFrame->linesize);
        
        I_AVYUVToBuffer(frame);
        RB_BindTexture(&texture);
        RB_UpdateTexture(&texture, videoBuffer);
    }
    
    av_free(frame);
}

//
// I_AVGetBufferProc
//

FF_DISABLE_DEPRECATION_WARNINGS

static int I_AVGetBufferProc(struct AVCodecContext *c, AVFrame *pic)
{
    int ret = avcodec_default_get_buffer2(c, pic, NULL);
    uint64_t *pts = av_malloc(sizeof(uint64_t));
    *pts = globalPts;

    pic->opaque = pts;
    return ret;
}

//
// I_AVReleaseBufferProc
//

static void I_AVReleaseBufferProc(struct AVCodecContext *c, AVFrame *pic)
{
    if(pic)
    {
        av_freep(&pic->opaque);
    }
#ifdef WIP
    avcodec_default_release_buffer(c, pic);
#endif
}

FF_ENABLE_DEPRECATION_WARNINGS

//=============================================================================
//
// Initialization and loading
//
//=============================================================================

//
// I_AVSetupCodecContext
//

static boolean I_AVSetupCodecContext(AVCodecContext **context, AVCodec **codec, int index)
{
    if(index < 0)
    {
        fprintf(stderr, "I_AVSetupCodecContext: Couldn't find stream\n");
        return false;
    }
    
    // get pointer to the codec context for the stream
    *context = formatCtx->streams[index]->id;
    
    // find the decoder for the stream
    *codec = avcodec_find_decoder((*context)->codec_id);
    
    if(*codec == NULL)
    {
        fprintf(stderr, "I_AVSetupCodecContext: Unsupported codec\n");
        return false;
    }

    // try to open codec
    if(avcodec_open2(*context, *codec, NULL) < 0)
    {
        fprintf(stderr, "I_AVSetupCodecContext: Couldn't open codec\n");
        return false;
    }

    return true;
}

//
// I_AVLoadVideo
//

static boolean I_AVLoadVideo(const char *filename)
{
    int i;
    int videoSize;
    int audioSize;
    char *filepath = I_GetUserFile(filename);
    
    // setup packet queues
    videoPacketQueue = I_AVAllocQueuePacket();
    audioPacketQueue = I_AVAllocQueuePacket();

    // setup audio queue
    audioQueue.mutex = SDL_CreateMutex();
    audioQueue.first = NULL;
    
    userExit = false;
    videoFinished = false;
    audioFinished = false;

    audioPts = 0;
    videoClock = 0;
    audioClock = 0;

    // not exactly looking for a wad, but this function makes
    // it easier to find our movie file
    if(!filepath)
    {
        return false;
    }
    
    if(avformat_open_input(&formatCtx, filepath, NULL, NULL))
    {
        fprintf(stderr, "I_AVLoadVideo: Couldn't load %s\n", filepath);
        return false;
    }
    
    if(avformat_find_stream_info(formatCtx, NULL))
    {
        fprintf(stderr, "I_AVLoadVideo: Couldn't find stream info\n");
        return false;
    }
    
#ifdef _DEBUG
    av_dump_format(formatCtx, 0, filepath, 0);
#endif
    
    videoStreamIdx = -1;
    audioStreamIdx = -1;
    
    // find the video and audio stream
    for(i = 0; i < (int)formatCtx->nb_streams; i++)
    {
        switch(formatCtx->streams[i]->codecpar->codec_type)
        {
            case AVMEDIA_TYPE_VIDEO:
                if(videoStreamIdx == -1)
                {
                    videoStreamIdx = i;
                }
                break;

            case AVMEDIA_TYPE_AUDIO:
                if(audioStreamIdx == -1)
                {
                    audioStreamIdx = i;
                }
                break;
                
            default:
                break;
        }
    }
    
    if(videoStreamIdx < 0)
    {
        fprintf(stderr, "I_AVLoadVideo: Couldn't find video stream\n");
        return false;
    }
    
    if(!I_AVSetupCodecContext(&videoCodecCtx, &videoCodec, videoStreamIdx) ||
       !I_AVSetupCodecContext(&audioCodecCtx, &audioCodec, audioStreamIdx))
    {
        return false;
    }

    reqWidth = videoCodecCtx->width;
    reqHeight = videoCodecCtx->height;

    // setup texture
    texture.colorMode = TCR_RGB;
    texture.origwidth = reqWidth;
    texture.origheight = reqHeight;
    texture.width = texture.origwidth;
    texture.height = texture.origheight;

FF_DISABLE_DEPRECATION_WARNINGS
    videoCodecCtx->get_buffer2 = I_AVGetBufferProc;
    videoCodecCtx->rc_buffer_size = I_AVReleaseBufferProc;
FF_ENABLE_DEPRECATION_WARNINGS
   
    videoFrame = av_frame_alloc();

    videoSize = av_image_get_buffer_size(AV_PIX_FMT_YUV444P, reqWidth, reqHeight, 22);//avpicture_get_size(AV_PIX_FMT_YUV444P, reqWidth, reqHeight);
    
    audioSize = MAX_AUDIO_FRAME_SIZE + AVPROBE_PADDING_SIZE; //AV_INPUT_BUFFER_PADDING_SIZE;

    videoBuffer = (uint8_t*)av_calloc(1, videoSize * sizeof(uint8_t));
    audioBuffer = (uint8_t*)av_calloc(1, audioSize * sizeof(uint8_t));
    RB_UploadTexture(&texture, videoBuffer, TC_CLAMP, TF_LINEAR);
    swsCtx = sws_getContext(videoCodecCtx->width,
                            videoCodecCtx->height,
                            videoCodecCtx->pix_fmt,
                            reqWidth,
                            reqHeight,
                            AV_PIX_FMT_YUV444P,
                            SWS_BICUBIC,
                            NULL, NULL, NULL);
    // assign appropriate parts of buffer to image planes in videoFrame
    // Note that videoFrame is an AVFrame, but AVFrame is a superset
    // of AVPicture
    av_image_fill_arrays((AVFrame*)videoFrame,
        videoBuffer,
        NULL,
        AV_PIX_FMT_YUV444P,
        reqWidth,
        reqHeight, NULL);
    currentPts = av_gettime();
    frameTime = 1000.0 / av_q2d(videoCodecCtx->framerate);
    lastFrameTime = frameTime;
    return true;
}

//=============================================================================
//
// General Functions
//
//=============================================================================

//
// I_AVShutdown
//

static void I_AVShutdown(void)
{
    //if(hasAudio)
    //{
    //    Mix_SetPostMix(NULL, NULL);
    //}

    SDL_WaitThread(thread, NULL);

    I_AVDeletePacketQueue(&videoPacketQueue);
    I_AVDeletePacketQueue(&audioPacketQueue);
    I_AVDeleteAudioQueue();

    av_free(videoBuffer);
    av_free(audioBuffer);
    av_free(videoFrame);
    
    avcodec_close(videoCodecCtx);
    avcodec_close(audioCodecCtx);
    avformat_close_input(&formatCtx);
    sws_freeContext(swsCtx);
    RB_DeleteTexture(&texture);
}

//
// I_AVDrawVideoStream
//

static void I_AVDrawVideoStream(void)
{
    SDL_Surface *screen;
    int ws, hs;
    
    glClearColor(0, 0, 0, 1);
    RB_ClearBuffer(GLCB_COLOR);
    I_AVProcessNextVideoFrame();

    screen = SDL_GetWindowSurface(window);
    ws = screen->w;
    hs = screen->h;
    RB_SetMaxOrtho(ws, hs);
    RB_DrawScreenTexture(&texture, reqWidth, reqHeight);

#ifdef USE_GLFW
    glfwSwapBuffers(Window);
#else
    SDL_GL_SwapWindow(window);
#endif
}

//
// I_AVCapFrameRate
//

static boolean I_AVCapFrameRate(void)
{
    uint64_t curTics = av_gettime();
    double elapsed = (double)(curTics - currentPts) / 1000.0;

    if(elapsed < frameTime)
    {
        if(frameTime - elapsed > 3.0)
        {
            // give up a small timeslice
            I_Sleep(2);
        }
        
        return true;
    }
    
    return false;
}

//
// I_AVIteratePacketsThread
//

static int SDLCALL I_AVIteratePacketsThread(void *param)
{
    AVPacket packet;

    //if(hasAudio)
    //{
        // start the post mix thread here
    //    Mix_SetPostMix(I_AVPostMixCallback, audioCodecCtx);
    //}
    
    while(av_read_frame(formatCtx, &packet) >= 0 && !userExit)
    {
        if(packet.stream_index == videoStreamIdx)
        {
            // queue the video packet
            I_AVPushPacketToQueue(videoPacketQueue, &packet);
        }
        else if(hasAudio && packet.stream_index == audioStreamIdx)
        {
            if(audioCodecCtx->sample_fmt == AV_SAMPLE_FMT_FLTP)
            {
                // queue the audio buffer from the packet
                I_AVFillAudioBuffer(&packet);
                av_packet_unref(&packet);
            }
        }
        else
        {
            av_packet_unref(&packet);
        }
    }

    // add end markers
    I_AVPushPacketToQueue(videoPacketQueue, NULL);
    I_AVPushPacketToQueue(audioPacketQueue, NULL);
    return 0;
}

//
// I_AVStartVideoStream
//

void I_AVStartVideoStream(const char *filename)
{
    static boolean avInitialized = false;
    SDL_Event ev;
   
    // initialize ffmpeg if it hasn't already
    if(!avInitialized)
    {
        hasAudio = true;

        av_muxer_iterate(NULL);
 
        avInitialized = true;
    }

    globalPts = AV_NOPTS_VALUE;
    
    if(!I_AVLoadVideo(filename))
    {
        // can't find or load the video... oh well..
        return;
    }
    
    thread = SDL_CreateThread(I_AVIteratePacketsThread, "I_AVIteratePacketsThread", NULL);

    while((!videoFinished || !audioFinished) && !userExit)
    {
        int joybuttons = I_StartTic;

        if(joybuttons > 0)
        {
            userExit = true;
            continue;
        }

        while(SDL_PollEvent(&ev))
        {
            switch(ev.type)
            {
                case SDL_KEYDOWN:
                    switch(ev.key.keysym.sym)
                    {
                    case SDLK_ESCAPE:
                    case SDLK_RETURN:
                    case SDLK_SPACE:
                        if(userPressed == false)
                        {
                            userExit = true;
                            userPressed = true;
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                case SDL_KEYUP:
                    userPressed = false;
                    break;
                case SDL_QUIT:
                    I_Quit();
                    break;
                default:
                    break;
            }
        }
        
        if(!I_AVCapFrameRate())
        {
            I_AVDrawVideoStream();
        }
    }
    
    I_AVShutdown();
    SDL_SetCursor(true);
    // make sure everything is unbinded
    RB_UnbindTexture();
}

#endif
