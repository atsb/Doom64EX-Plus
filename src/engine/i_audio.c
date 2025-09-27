// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2007-2012 Samuel Villarreal
// Copyright(C) 2024-2025 Gibbon
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION: Low-level audio API. Incorporates a sequencer system to
//              handle all sounds and music. All code related to the sequencer
//              is kept in it's own module and seperated from the rest of the
//              game code.
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#include <fmod.h>
#include <fmod_errors.h>

#include <SDL3/SDL_mutex.h>
#include <SDL3/SDL_timer.h>

#include "i_audio.h"
#include "doomtype.h"
#include "doomdef.h"
#include "doomstat.h"
#include "i_system.h"
#include "w_wad.h"
#include "z_zone.h"
#include "i_swap.h"
#include "con_console.h"
#include "d_player.h"
#include "tables.h"
#include "con_cvar.h"

// Thresholds for distance checks in game units
static const float GAME_SFX_MAX_UNITS_THRESHOLD = 1200.0f;
static const float GAME_SFX_FORCED_FULL_VOL_UNITS_THRESHOLD = 400.0f;

// ingame indexes for songs and sfx
static int* g_song_index_for_lump = NULL;
static int* g_sfx_index_for_lump = NULL;

static float I_Audio_CalculateDistanceToListener(mobj_t* sound_origin_mobj) {
    if (!sound_origin_mobj) {
        return FLT_MAX;
    }

    player_t* listener_player = &players[consoleplayer];
    if (!listener_player || !listener_player->mo) {
        return FLT_MAX;
    }

    mobj_t* listener_mobj = listener_player->mo;

    float dx = (float)(sound_origin_mobj->x - listener_mobj->x) / FRACUNIT;
    float dy_depth = (float)(sound_origin_mobj->y - listener_mobj->y) / FRACUNIT;

    fixed_t emitter_center_z = sound_origin_mobj->z + (sound_origin_mobj->height >> 1);
    float dz_height = (float)(emitter_center_z - listener_player->viewz) / FRACUNIT;

    return sqrtf(dx * dx + dy_depth * dy_depth + dz_height * dz_height);
}

// 20120203 villsa - cvar for soundfont location
CVAR_EXTERNAL(s_sfxvol);
CVAR_EXTERNAL(s_musvol);
static FMOD_SOUND* currentMidiSound = NULL;

struct Sound sound;
// struct Reverb fmod_reverb;

// FMOD Studio
static float INCHES_PER_METER = 39.3701f;
int num_sfx;

// FMOD_REVERB_PROPERTIES reverb_prop = FMOD_PRESET_GENERIC;

// FMOD_VECTOR fmod_reverb_position = { -8.0f, 2.0f, 2.0f };
float min_dist = 1.0f;
float max_dist = 15.0f;

//
// Mutex
//
static SDL_Mutex* lock = NULL;
#define MUTEX_LOCK()    SDL_LockMutex(lock);
#define MUTEX_UNLOCK()  SDL_UnlockMutex(lock);

// 20120205 villsa - bool to determine if sequencer is ready or not
static int seqready = 0;

//
// DEFINES
//

#define MIDI_CHANNELS   240
#define MIDI_MESSAGE    0x07
#define MIDI_END        0x2f
#define MIDI_SET_TEMPO  0x51
#define MIDI_SEQUENCER  0x7f

//
// MIDI DATA DEFINITIONS
//
// These data should not be modified outside the
// audio thread unless they're being initialized
//

typedef struct {
    char        header[4];
    int         length;
    byte* data;
    byte        channel;
} track_t;

typedef struct {
    char        header[4];
    int         chunksize;
    short       type;
    word        ntracks;
    word        delta;
    byte* data;
    int         length;
    track_t* tracks;
    int         tempo;
    double      timediv;
} song_t;

//
// SEQUENCER CHANNEL
//
// Active channels play sound or whatever
// is being fed from the midi reader. This
// is where communication between the audio
// thread and the game could get dangerous
// as they both need to access and modify
// data. In order to avoid this, certain
// properties have been divided up for
// both the audio thread and game code only
//

typedef enum {
    CHAN_STATE_READY = 0,
    CHAN_STATE_PAUSED = 1,
    CHAN_STATE_ENDED = 2,
    MAXSTATETYPES
} chanstate_e;

typedef struct {
    // these should never be modified unless
    // they're initialized
    song_t* song;
    track_t* track;

    // channel id for identifying an active channel
    // used primarily by normal sounds
    byte        id;

    // these are both accessed by the
    // audio thread and the game code
    // and should lock and unlock
    // the mutex whenever these need
    // to be modified..
    float       volume;
    byte        pan;
    sndsrc_t* origin;
    int         depth;

    // accessed by the audio thread only
    byte        key;
    byte        velocity;
    byte* pos;
    byte* jump;
    int         tics;
    int         nexttic;
    int         lasttic;
    int         starttic;
    Uint32      starttime;
    Uint32      curtime;
    chanstate_e state;
    int         paused;

    // read by audio thread but only
    // modified by game code
    int         stop;
    float       basevol;
} channel_t;

static channel_t playlist[MIDI_CHANNELS];   // channels active in sequencer

//
// DOOM SEQUENCER
//
// the backbone of the sequencer system. handles
// global volume and panning for all sounds/tracks
// and holds the allocated list of midi songs.
// all data is modified through the game and read
// by the audio thread.
//

typedef enum {
    SEQ_SIGNAL_IDLE = 0,    // idles. does nothing
    SEQ_SIGNAL_SHUTDOWN,        // signal the sequencer to shutdown, cleaning up anything in the process
    SEQ_SIGNAL_READY,           // sequencer will read and play any midi track fed to it
    SEQ_SIGNAL_RESET,
    SEQ_SIGNAL_PAUSE,
    SEQ_SIGNAL_RESUME,
    SEQ_SIGNAL_STOPALL,
    SEQ_SIGNAL_SETGAIN,         // signal the sequencer to update output gain
    MAXSIGNALTYPES
} seqsignal_e;

typedef union {
    sndsrc_t* valsrc;
    int         valint;
    float       valfloat;
} seqmessage_t;

typedef struct {
    int                     sfont_id; // 20120112 bkw: needs to be signed
    SDL_Thread* thread;
    int                   playtime;

    int                   voices;

    // tweakable settings for the sequencer
    float                   musicvolume;
    float                   soundvolume;

    // keep track of midi songs
    song_t* songs;
    int                     nsongs;

    seqmessage_t            message[3];

    // game code signals the sequencer to do stuff. game will
    // wait (while loop) until the audio thread signals itself
    // to be ready again
    // MP2E 12102013 - Only change using Seq_SetStatus
    seqsignal_e             signal;

    // 20120316 villsa - gain property (tweakable)
    float                   gain;
} doomseq_t;

static doomseq_t doomseq = { 0 };   // doom sequencer

typedef void(*eventhandler)(doomseq_t*, channel_t*);
typedef int(*signalhandler)(doomseq_t*);

static void FMOD_ERROR_CHECK(FMOD_RESULT result) {
    if (result != FMOD_OK) {
        fprintf(stderr, "FMOD Studio: %s\n", FMOD_ErrorString(result));
        // useful for crashing as soon as there is an fmod error
        //
        // compile program with LDFLAGS="-fsanitize=address" CFLAGS="-O0 -g -fsanitize=address  -fno-omit-frame-pointer"
        // to get a nice backtrace with line numbers
        //
        // *((int *)NULL) = 0xDEADC0DE;
    }
}

static void StopChannel(FMOD_CHANNEL**channel) {
    if(*channel) {
        FMOD_ERROR_CHECK(FMOD_Channel_Stop(*channel));
        *channel = NULL;
    }
}

static void ReleaseSound(FMOD_SOUND **sound) {
    if (*sound) {
        FMOD_ERROR_CHECK(FMOD_Sound_Release(*sound));
        *sound = NULL;
    }
}


//
// Seq_SetGain
//
// Set the 'master' volume for the sequencer. Affects
// all sounds that are played
//

void Seq_SetGain(float db) {
    // FIXME: incomplete ?
    if(sound.fmod_studio_channel_loop) {
        FMOD_ERROR_CHECK(FMOD_Channel_SetLowPassGain(sound.fmod_studio_channel_loop, db));
    }
}

//
// Seq_SetConfig
//

static void Seq_SetConfig(doomseq_t* seq, char* setting, int value) {

}

//
// Song_GetTimeDivision
//

static double Song_GetTimeDivision(song_t* song) {
    return (double)song->tempo / (double)song->delta / 1000.0;
}

//
// Seq_SetStatus
//

static void Seq_SetStatus(doomseq_t* seq, int status) {
    MUTEX_LOCK()
        seq->signal = status;
    MUTEX_UNLOCK()
}

//
// Seq_WaitOnSignal
//

/*static void Seq_WaitOnSignal(doomseq_t* seq)
{
    while(1)
    {
        if(seq->signal == SEQ_SIGNAL_READY)
            break;
    }
}*/

//
// Chan_SetMusicVolume
//
// Should be set by the audio thread
//

void Chan_SetMusicVolume(float volume) {
    FMOD_ERROR_CHECK(FMOD_System_GetMasterChannelGroup(sound.fmod_studio_system_music, &sound.master_music));
    FMOD_ERROR_CHECK(FMOD_ChannelGroup_SetVolume(sound.master_music, volume / 255.0f));
}

//
// Chan_SetSoundVolume
//
// Should be set by the audio thread
//

void Chan_SetSoundVolume(float volume) {
    FMOD_ERROR_CHECK(FMOD_System_GetMasterChannelGroup(sound.fmod_studio_system, &sound.master));
    FMOD_ERROR_CHECK(FMOD_ChannelGroup_SetVolume(sound.master, volume / 255.0f));
}

//
// Chan_GetNextMidiByte
//
// Gets the next byte in a midi track
//

static byte Chan_GetNextMidiByte(channel_t* chan) {
    if ((int)(chan->pos - chan->song->data) >= chan->song->length) {
        I_Error("Chan_GetNextMidiByte: Unexpected end of track");
    }

    return *chan->pos++;
}

//
// Chan_CheckTrackEnd
//
// Checks if the midi reader has reached the end
//

static int Chan_CheckTrackEnd(channel_t* chan) {
    return ((int)(chan->pos - chan->song->data) >= chan->song->length);
}

//
// Chan_GetNextTick
//
// Read the midi track to get the next delta time
//

static int Chan_GetNextTick(channel_t* chan) {
    int tic;
    int i;

    tic = Chan_GetNextMidiByte(chan);
    if (tic & 0x80) {
        byte mb;

        tic = tic & 0x7f;

        //
        // the N64 version loops infinitely but since the
        // delta time can only be four bytes long, just loop
        // for the remaining three bytes..
        //
        for (i = 0; i < 3; i++) {
            mb = Chan_GetNextMidiByte(chan);
            tic = (mb & 0x7f) + (tic << 7);

            if (!(mb & 0x80)) {
                break;
            }
        }
    }

    return (chan->starttic + (int)((double)tic * chan->song->timediv));
}

//
// Song_ClearPlaylist
//

static void Song_ClearPlaylist(void) {
    int i;

    for (i = 0; i < MIDI_CHANNELS; i++) {
        dmemset(&playlist[i], 0, sizeof(song_t));

        playlist[i].id = i;
        playlist[i].state = CHAN_STATE_READY;
    }
}

//
// Chan_RemoveTrackFromPlaylist
//

static int Chan_RemoveTrackFromPlaylist(doomseq_t* seq, channel_t* chan) {
    if (!chan->song || !chan->track) {
        return false;
    }

    chan->song = NULL;
    chan->track = NULL;
    chan->jump = NULL;
    chan->tics = 0;
    chan->nexttic = 0;
    chan->lasttic = 0;
    chan->starttic = 0;
    chan->curtime = 0;
    chan->starttime = 0;
    chan->pos = 0;
    chan->key = 0;
    chan->velocity = 0;
    chan->depth = 0;
    chan->state = CHAN_STATE_ENDED;
    chan->paused = false;
    chan->stop = false;
    chan->volume = 0.0f;
    chan->basevol = 0.0f;
    chan->pan = 0;
    chan->origin = NULL;

    seq->voices--;

    return true;
}

//
// Event_NoteOff
//

static void Event_NoteOff(doomseq_t* seq, channel_t* chan) {
    chan->key = Chan_GetNextMidiByte(chan);
    chan->velocity = 0;
}

//
// Event_NoteOn
//

static void Event_NoteOn(doomseq_t* seq, channel_t* chan) {
    chan->key = Chan_GetNextMidiByte(chan);
    chan->velocity = Chan_GetNextMidiByte(chan);
}

//
// Event_ControlChange
//

static void Event_ControlChange(doomseq_t* seq, channel_t* chan) {
    int ctrl;
    int val;

    ctrl = Chan_GetNextMidiByte(chan);
    val = Chan_GetNextMidiByte(chan);

    if (ctrl == 0x07) {  // update volume
        if (chan->song->type == 1) {
            chan->volume = ((float)val * seq->musicvolume) / 127.0f;
            //Chan_SetMusicVolume(seq, chan);
        }
        else {
            chan->volume = ((float)val * chan->volume) / 127.0f;
            //Chan_SetSoundVolume(seq, chan);
        }
    }
}

//
// Event_ProgramChange
//

static void Event_ProgramChange(doomseq_t* seq, channel_t* chan) {
    Chan_GetNextMidiByte(chan);
}

//
// Event_ChannelPressure
//

static void Event_ChannelPressure(doomseq_t* seq, channel_t* chan) {
    Chan_GetNextMidiByte(chan);
}

//
// Event_PitchBend
//

static void Event_PitchBend(doomseq_t* seq, channel_t* chan) {
    Chan_GetNextMidiByte(chan);
    Chan_GetNextMidiByte(chan);
}

//
// Event_Meta
//

static void Event_Meta(doomseq_t* seq, channel_t* chan) {
    int meta;
    int b;
    int i;
    char string[256];

    meta = Chan_GetNextMidiByte(chan);

    switch (meta) {
        // mostly for debugging/logging
    case MIDI_MESSAGE:
        b = Chan_GetNextMidiByte(chan);
        dmemset(string, 0, 256);

        for (i = 0; i < b; i++) {
            string[i] = Chan_GetNextMidiByte(chan);
        }

        string[b + 1] = '\n';
        break;

    case MIDI_END:
        b = Chan_GetNextMidiByte(chan);
        Chan_RemoveTrackFromPlaylist(seq, chan);
        break;

    case MIDI_SET_TEMPO:
        b = Chan_GetNextMidiByte(chan);   // length

        if (b != 3) {
            return;
        }

        chan->song->tempo =
            (Chan_GetNextMidiByte(chan) << 16) |
            (Chan_GetNextMidiByte(chan) << 8) |
            (Chan_GetNextMidiByte(chan) & 0xff);

        if (chan->song->tempo == 0) {
            return;
        }

        chan->song->timediv = Song_GetTimeDivision(chan->song);
        chan->starttime = chan->curtime;
        break;

        // game-specific midi event
    case MIDI_SEQUENCER:
        b = Chan_GetNextMidiByte(chan);   // length
        b = Chan_GetNextMidiByte(chan);   // manufacturer (should be 0)
        if (!b) {
            b = Chan_GetNextMidiByte(chan);
            if (b == 0x23) {
                // set jump position
                chan->jump = chan->pos;
            }
            else if (b == 0x20) {
                b = Chan_GetNextMidiByte(chan);
                b = Chan_GetNextMidiByte(chan);

                // goto jump position
                if (chan->jump) {
                    chan->pos = chan->jump;
                }
            }
        }
        break;

    default:
        break;
    }
}

static const eventhandler seqeventlist[7] = {
    Event_NoteOff,
    Event_NoteOn,
    NULL,
    Event_ControlChange,
    Event_ProgramChange,
    Event_ChannelPressure,
    Event_PitchBend
};

//
// Signal_Idle
//

static int Signal_Idle(doomseq_t* seq) {
    return 0;
}

//
// Signal_Shutdown
//

static int Signal_Shutdown(doomseq_t* seq) {
    return -1;
}

//
// Signal_StopAll
//

static int Signal_StopAll(doomseq_t* seq) {
    channel_t* c;
    int i;

    for (i = 0; i < MIDI_CHANNELS; i++) {
        c = &playlist[i];

        if (c->song) {
            Chan_RemoveTrackFromPlaylist(seq, c);
        }
    }

    Seq_SetStatus(seq, SEQ_SIGNAL_READY);
    return 1;
}

//
// Signal_Reset
//

static int Signal_Reset(doomseq_t* seq) {

    Seq_SetStatus(seq, SEQ_SIGNAL_READY);
    return 1;
}

//
// Signal_Pause
//
// Pause all currently playing songs
//

static int Signal_Pause(doomseq_t* seq) {
    Seq_SetStatus(seq, SEQ_SIGNAL_READY);
    return 1;
}

//
// Signal_Resume
//
// Resume all songs that were paused
//

static int Signal_Resume(doomseq_t* seq) {
    Seq_SetStatus(seq, SEQ_SIGNAL_READY);
    return 1;
}

static const signalhandler seqsignallist[MAXSIGNALTYPES] = {
    Signal_Idle,
    Signal_Shutdown,
    NULL,
    Signal_Reset,
    Signal_Pause,
    Signal_Resume,
    Signal_StopAll
};

//
// Chan_CheckState
//

static int Chan_CheckState(doomseq_t* seq, channel_t* chan) {
    if (chan->state == CHAN_STATE_ENDED) {
        return true;
    }
    else if (chan->state == CHAN_STATE_READY && chan->paused) {
        chan->state = CHAN_STATE_PAUSED;
        chan->lasttic = chan->nexttic - chan->tics;
        return true;
    }
    else if (chan->state == CHAN_STATE_PAUSED) {
        if (!chan->paused) {
            chan->nexttic = chan->tics + chan->lasttic;
            chan->state = CHAN_STATE_READY;
        }
        else {
            return true;
        }
    }

    return false;
}

//
// Chan_RunSong
//
// Main midi parsing routine
//

static void Chan_RunSong(doomseq_t* seq, channel_t* chan, int msecs) {
    byte event;
    byte c;
    song_t* song;
    byte channel;
    track_t* track;

    song = chan->song;
    track = chan->track;

    //
    // get next tic
    //
    if (chan->starttime == 0) {
        chan->starttime = msecs;
    }

    // villsa 12292013 - try to get precise timing to avoid de-syncs
    chan->curtime = msecs;
    chan->tics += ((chan->curtime - chan->starttime) - chan->tics);

    if (Chan_CheckState(seq, chan)) {
        return;
    }

    //
    // keep parsing through midi track until
    // the end is reached or until it reaches next
    // delta time
    //
    while (chan->state != CHAN_STATE_ENDED) {
        if (chan->song->type == 0) {
            chan->volume = chan->basevol;
            //Chan_SetSoundVolume(seq, chan);
        }
        else {
            //Chan_SetMusicVolume(seq, chan);
        }

        //
        // not ready to execute events yet
        //
        if (chan->tics < chan->nexttic) {
            return;
        }

        chan->starttic = chan->nexttic;
        c = Chan_GetNextMidiByte(chan);

        if (c == 0xff) {
            Event_Meta(seq, chan);
        }
        else {
            eventhandler eventhandle;

            event = (c >> 4) - 0x08;
            channel = c & 0x0f;

            if (event >= 0 && event < 7) {
                //
                // for music, use the generic midi channel
                // but for sounds, use the assigned id
                //
                if (song->type >= 1) {
                    track->channel = channel;
                }
                else {
                    track->channel = chan->id;
                }

                eventhandle = seqeventlist[event];

                if (eventhandle != NULL) {
                    eventhandle(seq, chan);
                }
            }
        }

        //
        // check for end of the track, otherwise get
        // the next delta time
        //
        if (chan->state != CHAN_STATE_ENDED) {
            if (Chan_CheckTrackEnd(chan)) {
                chan->state = CHAN_STATE_ENDED;
            }
            else {
                chan->nexttic = Chan_GetNextTick(chan);
            }
        }
    }
}

//
// Seq_RunSong
//

static void Seq_RunSong(doomseq_t* seq, int msecs) {
    int i;
    channel_t* chan;

    seq->playtime = msecs;

    for (i = 0; i < MIDI_CHANNELS; i++) {
        chan = &playlist[i];

        if (!chan->song) {
            continue;
        }

        if (chan->stop) {
            Chan_RemoveTrackFromPlaylist(seq, chan);
        }
        else {
            Chan_RunSong(seq, chan, msecs);
        }
    }
}

//
// Song_RegisterTracks
//
// Allocate data for all tracks for a midi song
//

static int Song_RegisterTracks(song_t* song) {
    int i;
    byte* data;

    song->tracks = (track_t*)Z_Calloc(sizeof(track_t) * song->ntracks, PU_STATIC, 0);
    data = song->data + 0x0e;

    for (i = 0; i < song->ntracks; i++) {
        track_t* track = &song->tracks[i];

        dmemcpy(track, data, 8);
        if (dstrncmp(track->header, "MTrk", 4)) {
            return false;
        }

        data = data + 8;

        track->length = I_SwapBE32(track->length);
        track->data = data;

        data = data + track->length;
    }

    return true;
}

//
// Seq_RegisterSongs
//
// Allocate data for all midi songs
//

static int Seq_RegisterSongs(doomseq_t* seq) {
    int i, fail = 0, total = 0;

    seq->nsongs = 0;

    g_song_index_for_lump = (int*)Z_Realloc(g_song_index_for_lump, sizeof(int) * numlumps, PU_STATIC, 0);
    for (i = 0; i < numlumps; ++i) g_song_index_for_lump[i] = -1;

    for (i = 0; i < numlumps; ++i) {
        int len = W_LumpLength(i);
        if (len < 14) 
            continue;

        const unsigned char* p = (const unsigned char*)W_CacheLumpNum(i, PU_STATIC);
        if (!p) 
            continue;

        if (!dstrncmp((const char*)p, "MThd", 4)) { 
            ++total; 
            continue; 
        }

        if (len >= 12 && !dstrncmp((const char*)p, "RIFF", 4) && !dstrncmp((const char*)p + 8, "RMID", 4)) {
            int off; int found = 0;
            for (off = 12; off <= len - 4; ++off) {
                if (!dstrncmp((const char*)p + off, "MThd", 4)) { 
                    found = 1; 
                    break; 
                }
            }
            if (found) ++total;
        }
    }

    if (total <= 0) {
        seq->songs = NULL;
        seq->nsongs = 0;
        return true;
    }

    seq->songs = (song_t*)Z_Calloc(total * sizeof(song_t), PU_STATIC, 0);
    seq->nsongs = 0;

    for (i = 0; i < numlumps; ++i) {
        int len = W_LumpLength(i);
        if (len < 14) 
            continue;

        const unsigned char* base = (const unsigned char*)W_CacheLumpNum(i, PU_STATIC);
        if (!base) 
            continue;

        const unsigned char* midip = NULL;
        int midilen = 0;

        if (!dstrncmp((const char*)base, "MThd", 4)) {
            midip = base;
            midilen = len;
        }
        else if (len >= 12 && !dstrncmp((const char*)base, "RIFF", 4) && !dstrncmp((const char*)base + 8, "RMID", 4)) {
            int off; int found = 0;
            for (off = 12; off <= len - 4; ++off) {
                if (!dstrncmp((const char*)base + off, "MThd", 4)) { 
                    found = 1; midip = base + off; midilen = len - off; 
                    break; 
                }
            }
            if (!found) 
                continue;
        }
        else {
            continue;
        }

        song_t tmp; dmemset(&tmp, 0, sizeof(tmp));
        tmp.data = (void*)midip;
        tmp.length = midilen;

        dmemcpy(&tmp, tmp.data, 0x0e);
        if (dstrncmp(tmp.header, "MThd", 4)) { 
            fail++; 
            continue; 
        }

        tmp.chunksize = I_SwapBE32(tmp.chunksize);
        tmp.ntracks = I_SwapBE16(tmp.ntracks);
        tmp.delta = I_SwapBE16(tmp.delta);
        tmp.type = I_SwapBE16(tmp.type);
        tmp.timediv = Song_GetTimeDivision(&tmp);
        tmp.tempo = 480000;

        if (!Song_RegisterTracks(&tmp)) { 
            fail++; 
            continue; 
        }

        seq->songs[seq->nsongs] = tmp;
        g_song_index_for_lump[i] = seq->nsongs;
        seq->nsongs++;
    }

    if (fail) I_Printf("Failed to load %d MIDI tracks.\n", fail);
    return true;
}

//
// Seq_RegisterSounds
//
// Load sound effects from WAD lumps (between DS_START and DS_END)
//

static int Seq_RegisterSounds(void) {
    int i, success = 0, fail = 0;
    FMOD_RESULT result;

    memset(sound.fmod_studio_sound, 0, MAX_GAME_SFX * sizeof(FMOD_SOUND*));
    num_sfx = 0;

    g_sfx_index_for_lump = (int*)Z_Realloc(g_sfx_index_for_lump, sizeof(int) * numlumps, PU_STATIC, 0);
    for (i = 0; i < numlumps; ++i) 
        g_sfx_index_for_lump[i] = -1;
    for (i = 0; i < numlumps; ++i) {

        if (num_sfx >= MAX_GAME_SFX) {
            CON_Warnf("Seq_RegisterSounds: Exceeded limit of %d. Further SFX ignored.\n", MAX_GAME_SFX);
            fail += (numlumps - i);
            break;
        }

        int len = W_LumpLength(i);
        if (len <= 4) 
            continue;

        const unsigned char* p = (const unsigned char*)W_CacheLumpNum(i, PU_STATIC);
        if (!p) { 
            fail++; 
            continue; 
        }

        int is_audio = 0;
        int is_pcm = 0;          /* WAV/AIFF. WAV header size = 44 */
        FMOD_SOUND_TYPE hinted = FMOD_SOUND_TYPE_UNKNOWN;

        if (!is_audio && len >= 12 && !dstrncmp((const char*)p, "RIFF", 4) && !dstrncmp((const char*)p + 8, "WAVE", 4)) {
            is_audio = 1; is_pcm = 1; hinted = FMOD_SOUND_TYPE_WAV;
        }
        if (!is_audio && len >= 12 && !dstrncmp((const char*)p, "FORM", 4) &&
            (!dstrncmp((const char*)p + 8, "AIFF", 4) || !dstrncmp((const char*)p + 8, "AIFC", 4))) {
            is_audio = 1; is_pcm = 1; hinted = FMOD_SOUND_TYPE_AIFF;
        }
        if (!is_audio && len >= 4 && (!dstrncmp((const char*)p, "OggS", 4) || !dstrncmp((const char*)p, "fLaC", 4) ||
                !dstrncmp((const char*)p, "FSB5", 4) || !dstrncmp((const char*)p, "FSB4", 4))) {
            is_audio = 1;

            if (!dstrncmp((const char*)p, "OggS", 4)) 
                hinted = FMOD_SOUND_TYPE_OGGVORBIS;

            else if (!dstrncmp((const char*)p, "fLaC", 4)) 
                hinted = FMOD_SOUND_TYPE_FLAC;

            else hinted = FMOD_SOUND_TYPE_FSB;
        }
        if (!is_audio && len >= 3 && !dstrncmp((const char*)p, "ID3", 3)) { 
            is_audio = 1; 
            hinted = FMOD_SOUND_TYPE_MPEG; 
        }
        if (!is_audio && len >= 2) {

            // FF FB, FF F3, FF F2 : MPEG - 1 Layer 3 file without an ID3 tag or with an ID3v1 tag (which is appended at the end of the file)
            byte b0 = p[0], b1 = p[1];
            if (b0 == 0xFF && (b1 == 0xFB || b1 == 0xF3 || b1 == 0xF2)) {
                is_audio = 1; 
                hinted = FMOD_SOUND_TYPE_MPEG; 
            }
        }

        if (!is_audio) continue;

        FMOD_CREATESOUNDEXINFO exinfo; dmemset(&exinfo, 0, sizeof(exinfo));
        exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
        exinfo.length = len;
        exinfo.suggestedsoundtype = hinted;

        FMOD_MODE mode = FMOD_3D | FMOD_LOOP_OFF;
        FMOD_SOUND* snd = NULL;

        if (is_pcm) {
            if (hinted == FMOD_SOUND_TYPE_WAV && len <= 44) {
                // exclude NOSOUND WAV lump as it cannot be created
                result = FMOD_ERR_UNSUPPORTED;
            } else {
                result = FMOD_System_CreateSound(
                    sound.fmod_studio_system,
                    (const char*)p,
                    FMOD_OPENMEMORY_POINT | mode,
                    &exinfo,
                    &snd
                );
                FMOD_ERROR_CHECK(result);
                if (result != FMOD_OK) {
                    result = FMOD_System_CreateSound(
                        sound.fmod_studio_system,
                        (const char*)p,
                        FMOD_OPENMEMORY | mode,
                        &exinfo,
                        &snd
                    );
                    FMOD_ERROR_CHECK(result);
                }
            }
        }
        else {
            result = FMOD_System_CreateSound(
                sound.fmod_studio_system,
                (const char*)p,
                FMOD_OPENMEMORY | FMOD_CREATECOMPRESSEDSAMPLE | mode,
                &exinfo,
                &snd
            );
            FMOD_ERROR_CHECK(result);
            if (result != FMOD_OK) {
                result = FMOD_System_CreateSound(
                    sound.fmod_studio_system,
                    (const char*)p,
                    FMOD_OPENMEMORY | mode,
                    &exinfo,
                    &snd
                );
                FMOD_ERROR_CHECK(result);
                if (result != FMOD_OK) {
                    result = FMOD_System_CreateSound(
                        sound.fmod_studio_system,
                        (const char*)p,
                        FMOD_OPENMEMORY | FMOD_CREATESTREAM | mode,
                        &exinfo,
                        &snd
                    );
                    FMOD_ERROR_CHECK(result);
                }
            }
        }

        if (result == FMOD_OK && snd) {
            sound.fmod_studio_sound[num_sfx] = snd;
            {
                float min_d = GAME_SFX_FORCED_FULL_VOL_UNITS_THRESHOLD * INCHES_PER_METER;
                float max_d = GAME_SFX_MAX_UNITS_THRESHOLD * INCHES_PER_METER;
                FMOD_ERROR_CHECK(FMOD_Sound_Set3DMinMaxDistance(sound.fmod_studio_sound[num_sfx], min_d, max_d));
            }
            g_sfx_index_for_lump[i] = num_sfx;
            success++;
        }
        else {
            fail++;
        }
        num_sfx++;
    }

    if (fail > 0) 
        I_Printf("Failed to load %d sound effects.\n", fail);

    if (success > 0) 
        I_Printf("Successfully registered %d sound effects.\n", success);

    return true;
}

//
// Seq_Shutdown
//

static void Seq_Shutdown(doomseq_t* seq) {
    //
    // signal the sequencer to shut down
    //
    if(seq->thread) {
        Seq_SetStatus(seq, SEQ_SIGNAL_SHUTDOWN);
        SDL_WaitThread(seq->thread, NULL);
        seq->thread = NULL;
    }
}

//
// Thread_PlayerHandler
//
// Main routine of the audio thread
//

static int SDLCALL Thread_PlayerHandler(void* param) {
    doomseq_t* seq = (doomseq_t*)param;
    long long start = SDL_GetTicks();
    long long delay = 0;
    int status;
    long long count = 0;
    signalhandler signal;

    while (1) {

        //
        // check status of the sequencer
        //
        signal = seqsignallist[seq->signal];

        if (signal) {
            status = signal(seq);

            if (status == 0) {
                // villsa 12292013 - add a delay here so we don't
                // thrash the loop while idling
                SDL_Delay(1);
                continue;
            }

            if (status == -1) {
                return 1;
            }
        }

        //
        // play some songs
        //
        Seq_RunSong(seq, SDL_GetTicks() - start);
        count++;

        // try to avoid incremental time de-syncs
        delay = count - (SDL_GetTicks() - start);

        if (delay > 0) {
            SDL_Delay(delay);
        }
    }

    return 0;
}

//
// I_InitSequencer
//


void I_InitSequencer(void) {
    I_Printf("Audio Engine: FMOD Studio by Firelight Technologies Pty Ltd.\n");

    FMOD_ERROR_CHECK(FMOD_System_Create(&sound.fmod_studio_system, FMOD_VERSION));
    FMOD_ERROR_CHECK(FMOD_System_Create(&sound.fmod_studio_system_music, FMOD_VERSION));

    FMOD_ERROR_CHECK(FMOD_System_Init(sound.fmod_studio_system, 92, FMOD_INIT_3D_RIGHTHANDED | FMOD_INIT_PROFILE_ENABLE, NULL));
    FMOD_ERROR_CHECK(FMOD_System_Init(sound.fmod_studio_system_music, 128, FMOD_INIT_NORMAL, NULL));

    FMOD_ERROR_CHECK(FMOD_System_GetMasterChannelGroup(sound.fmod_studio_system, &sound.master));
    FMOD_ERROR_CHECK(FMOD_System_GetMasterChannelGroup(sound.fmod_studio_system_music, &sound.master_music));

    //
    // init mutex
    //
    lock = SDL_CreateMutex();
    if (lock == NULL) {
        CON_Warnf("I_InitSequencer: failed to create mutex");
        return;
    }

    dmemset(&doomseq, 0, sizeof(doomseq_t));

    //
    // init sequencer thread
    //

    // villsa 12292013 - I can't guarantee that this will resolve
    // the issue of the titlemap/intermission/finale music to be
    // off-sync when uncapped framerates are enabled but for some
    // reason, calling SDL_GetTicks before initalizing the thread
    // will reduce the chances of it happening
    SDL_GetTicks();

    doomseq.thread = SDL_CreateThread(Thread_PlayerHandler, "SynthPlayer", &doomseq);
    if (doomseq.thread == NULL) {
        CON_Warnf("I_InitSequencer: failed to create audio thread");
        return;
    }

    //
    // init settings
    //
    Seq_SetConfig(&doomseq, "synth.midi-channels", 0x10 + MIDI_CHANNELS);
    Seq_SetConfig(&doomseq, "synth.polyphony", 128); // [Immorpher] high polyphony slows down the game

    //
    // set state
    //
    float gain = 1.0f;

    Seq_SetStatus(&doomseq, SEQ_SIGNAL_READY);
    Seq_SetGain(gain);

    //
    // if something went terribly wrong, then shutdown everything
    //
    if (!Seq_RegisterSongs(&doomseq)) {
        CON_Warnf("I_InitSequencer: Failed to register songs\n");
        Seq_Shutdown(&doomseq);
        return;
    }

    //
    // if something went terribly wrong, then shutdown everything
    //
    if (!Seq_RegisterSounds()) {
        CON_Warnf("I_InitSequencer: Failed to register sounds\n");
        Seq_Shutdown(&doomseq);
        return;
    }

    //
    // where's the soundfont file? not found then shutdown everything
    //
    if (doomseq.sfont_id == -1) {
        CON_Warnf("I_InitSequencer: Failed to find soundfont file\n");
        Seq_Shutdown(&doomseq);
        return;
    }

    Song_ClearPlaylist();

    // 20120205 villsa - sequencer is now ready
    seqready = true;
}


void I_Update(void) {
    if(sound.fmod_studio_system) {
        FMOD_ERROR_CHECK(FMOD_System_Update(sound.fmod_studio_system));
    }
    if(sound.fmod_studio_system_music) {
        FMOD_ERROR_CHECK(FMOD_System_Update(sound.fmod_studio_system_music));
    }
}

//
// I_GetMaxChannels
//

int I_GetMaxChannels(void) {
    return MIDI_CHANNELS;
}

//
// I_GetVoiceCount
//

int I_GetVoiceCount(void) {
    return doomseq.voices;
}

//
// I_GetSoundSource
//

sndsrc_t* I_GetSoundSource(int c) {
    if (playlist[c].song == NULL) {
        return NULL;
    }

    return playlist[c].origin;
}

//
// I_RemoveSoundSource
//

void I_RemoveSoundSource(int c) {
    playlist[c].origin = NULL;
}

//
// I_UpdateChannel
//

void I_UpdateChannel(int c, int volume, int pan, fixed_t x, fixed_t y) {
    channel_t* chan;

    // FIXME: TODO
    
    //FMOD_VECTOR soundPosition = { x, y, 0.0f };  // Set the appropriate position in 3D space
    //FMOD_Channel_Set3DAttributes(sound.fmod_studio_channel, &soundPosition, NULL);

    //FMOD_Channel_SetMode(sound.fmod_studio_channel, FMOD_3D_WORLDRELATIVE);

    chan = &playlist[c];
    chan->basevol = (float)volume;
    chan->pan = (byte)(pan >> 1);
}

static void ShutdownSystem(FMOD_SYSTEM **system) {
    if(*system) {
        FMOD_ERROR_CHECK(FMOD_System_Close(*system));
        FMOD_ERROR_CHECK(FMOD_System_Release(*system));
        *system = NULL;
    }
}

//
// I_ShutdownSound
// Shutdown sound when player exits the game / error occurs

void I_ShutdownSound(void)
{
    // wait for seq thread to terminate
    Seq_Shutdown(&doomseq);

    for(int i = 0 ; i < num_sfx ; i++) {
        ReleaseSound(&sound.fmod_studio_sound[i]);
    }
    
    StopChannel(&sound.fmod_studio_channel_music);
    StopChannel(&sound.fmod_studio_channel_loop);
    StopChannel(&sound.fmod_studio_channel_plasma_loop);

    // must be done after stopping fmod_studio_channel_music
    ReleaseSound(&currentMidiSound);

    ShutdownSystem(&sound.fmod_studio_system);
    ShutdownSystem(&sound.fmod_studio_system_music);
}

//
// I_SetMusicVolume
//

void I_SetMusicVolume(float volume) {
    if(sound.fmod_studio_channel_music) {
        FMOD_ERROR_CHECK(FMOD_Channel_SetVolume(sound.fmod_studio_channel_music, volume / 255.0f));
    }
    doomseq.musicvolume = (volume * 0.925f);
}

//
// I_SetSoundVolume
//

void I_SetSoundVolume(float volume) {
    // FIXME: incomplete ?
    doomseq.soundvolume = (volume * 0.925f);
}

//
// I_ResetSound
//

void I_ResetSound(void) {

    if (!seqready) {
        return;
    }

    Seq_SetStatus(&doomseq, SEQ_SIGNAL_RESET);
}

//
// I_PauseSound
//

void I_PauseSound(void) {
    if (!seqready) {
        return;
    }

    // FIXME: incomplete ?
    Seq_SetStatus(&doomseq, SEQ_SIGNAL_PAUSE);
}

//
// I_ResumeSound
//

void I_ResumeSound(void) {
    if (!seqready) {
        return;
    }

    // FIXME: incomplete ?
    Seq_SetStatus(&doomseq, SEQ_SIGNAL_RESUME);
}

//
// I_SetGain
//

void I_SetGain(float db) {
    if (!seqready) {
        return;
    }

    doomseq.gain = db;

    // FIXME: incomplete ?
    if(sound.fmod_studio_channel_loop) {
        FMOD_ERROR_CHECK(FMOD_Channel_SetLowPassGain(sound.fmod_studio_channel_loop, db));
    }
    Seq_SetStatus(&doomseq, SEQ_SIGNAL_SETGAIN);
}

//
// I_UpdateListenerPosition
//
// Updates the 3D listener attributes for FMOD Studio based on player's position and orientation.
//

void I_UpdateListenerPosition(fixed_t player_world_x, fixed_t player_world_y_depth, fixed_t player_eye_world_z_height, angle_t view_angle) {
    if (!sound.fmod_studio_system) {
        return;
    }

    FMOD_VECTOR listener_pos;
    FMOD_VECTOR listener_vel = { 0.0f, 0.0f, 0.0f };
    FMOD_VECTOR listener_forward;
    FMOD_VECTOR listener_up = { 0.0f, 1.0f, 0.0f };

    listener_pos.x = (float)player_world_x / FRACUNIT * INCHES_PER_METER;
    listener_pos.y = (float)player_eye_world_z_height / FRACUNIT * INCHES_PER_METER;
    listener_pos.z = (float)player_world_y_depth / FRACUNIT * INCHES_PER_METER;

    listener_forward.x = -(float)finecosine[view_angle >> ANGLETOFINESHIFT] / FRACUNIT;
    listener_forward.z = -(float)finesine[view_angle >> ANGLETOFINESHIFT] / FRACUNIT;
    listener_forward.y = 0.0f;

    FMOD_ERROR_CHECK(FMOD_System_Set3DListenerAttributes(sound.fmod_studio_system, 0, &listener_pos, &listener_vel, &listener_forward, &listener_up));
}

// FMOD Studio SFX API

int FMOD_StartSound(int sfx_id, sndsrc_t* origin, int volume, int pan) {
    FMOD_CHANNEL* sfx_channel = NULL;
    FMOD_RESULT result;

    if (!seqready || !sound.fmod_studio_system) {
        return -1;
    }

    if (sfx_id < 0 || sfx_id >= num_sfx || !sound.fmod_studio_sound[sfx_id]) {
        CON_Warnf("FMOD_StartSound: Invalid sfx_id %d or sound not loaded (num_sfx: %d)\n", sfx_id, num_sfx);
        return -1;
    }

    result = FMOD_System_PlaySound(sound.fmod_studio_system, sound.fmod_studio_sound[sfx_id], sound.master, true, &sfx_channel);
    if (result != FMOD_OK) {
        FMOD_ERROR_CHECK(result);
        return -1;
    }

    if(!sfx_channel) {
        CON_Warnf("FMOD_StartSound: PlaySound OK (default path) but channel is NULL (sfx_id %d)\n", sfx_id);
        return -1;
    }

    float instance_vol = (float)volume / 255.0f;
    float global_sfx_vol = (s_sfxvol.value >= 0 && s_sfxvol.value <= 255) ? (float)s_sfxvol.value / 255.0f : 1.0f;
    float final_volume = instance_vol * global_sfx_vol;
    if (isnan(final_volume) || isinf(final_volume)) final_volume = 0.75f;
    if (final_volume < 0.0f) final_volume = 0.0f;
    
    // Clamp to a maximum of 1.0 to prevent clipping (1.0f is really loud for FMOD)
    if (final_volume > 1.0f) {
        final_volume = 1.0f;
    }
    FMOD_ERROR_CHECK(FMOD_Channel_SetVolume(sfx_channel, final_volume));

    if (origin) {
        mobj_t* mobj = (mobj_t*)origin;
        FMOD_VECTOR pos, vel;
        
        pos.x = (float)mobj->x / FRACUNIT * INCHES_PER_METER;
        pos.y = (float)mobj->z / FRACUNIT * INCHES_PER_METER;
        pos.z = (float)mobj->y / FRACUNIT * INCHES_PER_METER;
        
        vel.x = 0.0f; vel.y = 0.0f; vel.z = 0.0f;
        
        FMOD_ERROR_CHECK(FMOD_Channel_Set3DAttributes(sfx_channel, &pos, &vel));
        FMOD_ERROR_CHECK(FMOD_Channel_SetMode(sfx_channel, FMOD_3D_WORLDRELATIVE));
    } else {
        float fmod_pan = ((float)pan - 128.0f) / 128.0f;
        if (isnan(fmod_pan) || isinf(fmod_pan)) fmod_pan = 0.0f;
        fmod_pan = (fmod_pan < -1.0f) ? -1.0f : (fmod_pan > 1.0f) ? 1.0f : fmod_pan;
        FMOD_ERROR_CHECK(FMOD_Channel_SetPan(sfx_channel, fmod_pan));
        FMOD_ERROR_CHECK(FMOD_Channel_SetMode(sfx_channel, FMOD_2D));
    }
    
    FMOD_ERROR_CHECK(FMOD_Channel_SetPaused(sfx_channel, false));

    if (origin) {
        float distance = I_Audio_CalculateDistanceToListener((mobj_t*)origin);

        if (distance > GAME_SFX_MAX_UNITS_THRESHOLD) {
            return -1;
        }

        if (distance <= GAME_SFX_FORCED_FULL_VOL_UNITS_THRESHOLD) {
            result = FMOD_System_PlaySound(sound.fmod_studio_system, sound.fmod_studio_sound[sfx_id], sound.master, true, &sfx_channel);

            if (result != FMOD_OK) {
                FMOD_ERROR_CHECK(result);
                return -1;
            }

            if (sfx_channel) {
                float instance_vol = (float)volume / 255.0f;
                float global_sfx_vol = (s_sfxvol.value >= 0 && s_sfxvol.value <= 255) ? (float)s_sfxvol.value / 255.0f : 1.0f;
                float final_volume = instance_vol * global_sfx_vol;
                if (isnan(final_volume) || isinf(final_volume)) final_volume = 0.75f;
                if (final_volume < 0.0f) final_volume = 0.0f;

                // Clamp to a maximum of 1.0 to prevent clipping (same reason)
                if (final_volume > 1.0f) {
                    final_volume = 1.0f;
                }
                FMOD_ERROR_CHECK(FMOD_Channel_SetVolume(sfx_channel, final_volume));
                FMOD_ERROR_CHECK(FMOD_Channel_SetPan(sfx_channel, 0.0f));
                FMOD_ERROR_CHECK(FMOD_Channel_SetMode(sfx_channel, FMOD_2D));
                FMOD_ERROR_CHECK(FMOD_Channel_SetPaused(sfx_channel, false));
                return sfx_id;
            }

            CON_Warnf("FMOD_StartSound: PlaySound OK (2D override) but channel is NULL (sfx_id %d)\n", sfx_id);
            return -1;
        }
    }
    return sfx_id;
}

int FMOD_StartSFXLoop(int sfx_id, int volume) {
    StopChannel(&sound.fmod_studio_channel_loop);

    if (!seqready || !sound.fmod_studio_system) {
        return -1;
    }
    
    if (sfx_id < 0 || sfx_id >= num_sfx || !sound.fmod_studio_sound[sfx_id]) {
        CON_Warnf("FMOD_StartSFXLoop: Invalid sfx_id %d or sound not loaded (num_sfx: %d)\n", sfx_id, num_sfx);
        return -1;
    }

    float instance_vol = (float)volume / 255.0f;
    float global_sfx_vol = (s_sfxvol.value >= 0 && s_sfxvol.value <= 255) ? (float)s_sfxvol.value / 255.0f : 1.0f;
    float final_volume = instance_vol * global_sfx_vol;

    if (isnan(final_volume) || isinf(final_volume)) final_volume = 0.75f;
    if (final_volume < 0.0f) final_volume = 0.0f;

    final_volume = final_volume * 2.0f;
    if (final_volume > 1.0f) {
        final_volume = 1.0f;
    }

    FMOD_RESULT result = FMOD_System_PlaySound(sound.fmod_studio_system,
        sound.fmod_studio_sound[sfx_id],
        sound.master,
        true,
        &sound.fmod_studio_channel_loop);
    FMOD_ERROR_CHECK(result);

    if (result == FMOD_OK && sound.fmod_studio_channel_loop) {
        FMOD_ERROR_CHECK(FMOD_Channel_SetVolume(sound.fmod_studio_channel_loop, final_volume));
        FMOD_ERROR_CHECK(FMOD_Channel_SetMode(sound.fmod_studio_channel_loop, FMOD_LOOP_NORMAL | FMOD_2D));
        FMOD_ERROR_CHECK(FMOD_Channel_SetVolumeRamp(sound.fmod_studio_channel_loop, false));
        FMOD_ERROR_CHECK(FMOD_Channel_SetPaused(sound.fmod_studio_channel_loop, false)); // Unpause
        return sfx_id;
    }
    
    CON_Warnf("FMOD_StartSFXLoop: Failed to play sound or get channel (sfx_id %d)\n", sfx_id);
    return -1;
}

void FMOD_StopPlasmaLoop(void) {
    StopChannel(&sound.fmod_studio_channel_plasma_loop);
}

int FMOD_StopSFXLoop(void) {
    StopChannel(&sound.fmod_studio_channel_loop);
    return 0;
}

int FMOD_StartPlasmaLoop(int sfx_id, int volume) {
    FMOD_StopPlasmaLoop();

    if (!seqready || !sound.fmod_studio_system) {
        return -1;
    }

    if (sfx_id < 0 || sfx_id >= num_sfx || !sound.fmod_studio_sound[sfx_id]) {
        CON_Warnf("FMOD_StartPlasmaLoop: Invalid sfx_id %d or sound not loaded (num_sfx: %d)\n", sfx_id, num_sfx);
        return -1;
    }

    FMOD_RESULT result = FMOD_System_PlaySound(sound.fmod_studio_system,
        sound.fmod_studio_sound[sfx_id],
        sound.master,
        true,
        &sound.fmod_studio_channel_plasma_loop);
    FMOD_ERROR_CHECK(result);

    if (result == FMOD_OK && sound.fmod_studio_channel_plasma_loop) {
        FMOD_ERROR_CHECK(FMOD_Channel_SetVolume(sound.fmod_studio_channel_plasma_loop, 0.1f));
        FMOD_ERROR_CHECK(FMOD_Channel_SetMode(sound.fmod_studio_channel_plasma_loop, FMOD_LOOP_NORMAL | FMOD_2D));
        FMOD_ERROR_CHECK(FMOD_Channel_SetVolumeRamp(sound.fmod_studio_channel_plasma_loop, false));
        FMOD_ERROR_CHECK(FMOD_Channel_SetPaused(sound.fmod_studio_channel_plasma_loop, false));
        return sfx_id;
    }
  
    CON_Warnf("FMOD_StartPlasmaLoop: Failed to play sound or get channel (sfx_id %d)\n", sfx_id);
    sound.fmod_studio_channel_plasma_loop = NULL;
    return -1;
}

void FMOD_StopSound(sndsrc_t* origin, int sfx_id) {
    // FIXME: TODO
    // I_Printf("FIXME: FMOD_StopSound unimplemented\n");
}

int FMOD_StartMusic(int mus_id) {
    FMOD_RESULT result;

    StopChannel(&sound.fmod_studio_channel_music);
    ReleaseSound(&currentMidiSound);

    float final_volume = 0.75f;
    if (s_musvol.value >= 0 && s_musvol.value <= 255) 
        final_volume = (float)s_musvol.value / 255.0f;

    if (isnan(final_volume) || isinf(final_volume)) 
        final_volume = 0.75f;

    if (final_volume < 0.0f) 
        final_volume = 0.0f;

    if (mus_id >= 0 && mus_id < doomseq.nsongs && doomseq.songs[mus_id].data != NULL) {
        FMOD_CREATESOUNDEXINFO exinfo; dmemset(&exinfo, 0, sizeof(exinfo));
        exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
        exinfo.length = doomseq.songs[mus_id].length;
        exinfo.dlsname = I_FindDataFile(DLS_FILENAME);

        result = FMOD_System_CreateSound(
            sound.fmod_studio_system_music,
            (const char*)doomseq.songs[mus_id].data,
            FMOD_OPENMEMORY | FMOD_LOOP_NORMAL | FMOD_2D,
            &exinfo,
            &currentMidiSound
        );
        FMOD_ERROR_CHECK(result);

        if (result == FMOD_OK && currentMidiSound) {
            result = FMOD_System_PlaySound(sound.fmod_studio_system_music, currentMidiSound, 
                sound.master_music, false, &sound.fmod_studio_channel_music);
            FMOD_ERROR_CHECK(result);
            if (result == FMOD_OK) {
                FMOD_ERROR_CHECK(FMOD_Channel_SetVolume(sound.fmod_studio_channel_music, final_volume));
                FMOD_ERROR_CHECK(FMOD_Channel_SetVolumeRamp(sound.fmod_studio_channel_music, false));
                return mus_id;
            }
            CON_Warnf("FMOD_StartMusic: Failed to play MIDI slot %d.\n", mus_id);
            ReleaseSound(&currentMidiSound);
            StopChannel(&sound.fmod_studio_channel_music);
            return -1;
        }
        CON_Warnf("FMOD_StartMusic: Failed to create MIDI for slot %d. Result: %d\n", mus_id, result);
        ReleaseSound(&currentMidiSound);
        return -1;
    }

    if (mus_id >= 0 && mus_id < numlumps) {
        const unsigned char* p = (const unsigned char*)W_CacheLumpNum(mus_id, PU_STATIC);
        int len = W_LumpLength(mus_id);
        if (!p || len < 4) { 
            CON_Warnf("FMOD_StartMusic: Lump %d invalid/empty.\n", mus_id); 
        return -1; 
        }

        if (len >= 4 && !dstrncmp((const char*)p, "MThd", 4)) {
            int slot = (g_song_index_for_lump && mus_id < numlumps) ? g_song_index_for_lump[mus_id] : -1;

            if (slot >= 0) 
                return FMOD_StartMusic(slot);

            FMOD_CREATESOUNDEXINFO exinfo; dmemset(&exinfo, 0, sizeof(exinfo));
            exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
            exinfo.length = len;
            exinfo.dlsname = I_FindDataFile(DLS_FILENAME);
            result = FMOD_System_CreateSound(sound.fmod_studio_system_music, (const char*)p, FMOD_OPENMEMORY | FMOD_LOOP_NORMAL | FMOD_2D, &exinfo, &currentMidiSound);
            FMOD_ERROR_CHECK(result);
            if (result == FMOD_OK && currentMidiSound) {
                result = FMOD_System_PlaySound(sound.fmod_studio_system_music, currentMidiSound, sound.master_music, false, &sound.fmod_studio_channel_music);
                FMOD_ERROR_CHECK(result);
                if (result == FMOD_OK) {
                    FMOD_ERROR_CHECK(FMOD_Channel_SetVolume(sound.fmod_studio_channel_music, final_volume));
                    FMOD_ERROR_CHECK(FMOD_Channel_SetVolumeRamp(sound.fmod_studio_channel_music, false));
                    return mus_id;
                }
            }
            CON_Warnf("FMOD_StartMusic: MIDI play failed for lump %d.\n", mus_id);
            ReleaseSound(&currentMidiSound);
            StopChannel(&sound.fmod_studio_channel_music);
            return -1;
        }

        if (len >= 12 && !dstrncmp((const char*)p, "RIFF", 4) && !dstrncmp((const char*)p + 8, "RMID", 4)) {
            int off; const unsigned char* midip = NULL; int midilen = 0;
            for (off = 12; off <= len - 4; ++off) {
                if (!dstrncmp((const char*)p + off, "MThd", 4)) { 
                    midip = p + off; 
                    midilen = len - off; 
                    break; 
                }
            }
            if (midip) {
                int slot = (g_song_index_for_lump && mus_id < numlumps) ? g_song_index_for_lump[mus_id] : -1;
                if (slot >= 0) return FMOD_StartMusic(slot);

                FMOD_CREATESOUNDEXINFO exinfo; dmemset(&exinfo, 0, sizeof(exinfo));
                exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
                exinfo.length = midilen;
                exinfo.dlsname = I_FindDataFile(DLS_FILENAME);
                result = FMOD_System_CreateSound(sound.fmod_studio_system_music, (const char*)midip, FMOD_OPENMEMORY | FMOD_LOOP_NORMAL | FMOD_2D, &exinfo, &currentMidiSound);
                FMOD_ERROR_CHECK(result);
                if (result == FMOD_OK && currentMidiSound) {
                    result = FMOD_System_PlaySound(sound.fmod_studio_system_music, currentMidiSound, sound.master_music, false, &sound.fmod_studio_channel_music);
                    FMOD_ERROR_CHECK(result);
                    if (result == FMOD_OK) {
                        FMOD_ERROR_CHECK(FMOD_Channel_SetVolume(sound.fmod_studio_channel_music, final_volume));
                        FMOD_ERROR_CHECK(FMOD_Channel_SetVolumeRamp(sound.fmod_studio_channel_music, false));
                        return mus_id;
                    }
                }
                // tell us when playing RMID files as MIDI files fails
                CON_Warnf("FMOD_StartMusic: RMID->MIDI play failed for lump %d.\n", mus_id);
                ReleaseSound(&currentMidiSound);
                StopChannel(&sound.fmod_studio_channel_music);
                return -1;
            }
        }
        {
            FMOD_MODE mode = FMOD_2D | FMOD_LOOP_NORMAL;
            FMOD_CREATESOUNDEXINFO exinfo; dmemset(&exinfo, 0, sizeof(exinfo));
            exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
            exinfo.length = len;
            exinfo.suggestedsoundtype = FMOD_SOUND_TYPE_UNKNOWN;

            // what to do when it is the format found
            if (len >= 4 && !dstrncmp((const char*)p, "OggS", 4))  exinfo.suggestedsoundtype = FMOD_SOUND_TYPE_OGGVORBIS;
            else if (len >= 4 && !dstrncmp((const char*)p, "fLaC", 4)) exinfo.suggestedsoundtype = FMOD_SOUND_TYPE_FLAC;
            else if (len >= 3 && !dstrncmp((const char*)p, "ID3", 3))  exinfo.suggestedsoundtype = FMOD_SOUND_TYPE_MPEG;
            else if (p[0] == 0xFF && (p[1] & 0xE0) == 0xE0)                exinfo.suggestedsoundtype = FMOD_SOUND_TYPE_MPEG;
            else if (len >= 4 && (!dstrncmp((const char*)p, "FSB5", 4) || !dstrncmp((const char*)p, "FSB4", 4))) exinfo.suggestedsoundtype = FMOD_SOUND_TYPE_FSB;

            FMOD_SOUND* stream = NULL;

            result = FMOD_System_CreateSound(
                sound.fmod_studio_system_music,
                (const char*)p,
                FMOD_OPENMEMORY | FMOD_CREATESTREAM | mode,
                &exinfo,
                &stream
            );
            FMOD_ERROR_CHECK(result);

            if (result != FMOD_OK) {
                result = FMOD_System_CreateSound(
                    sound.fmod_studio_system_music,
                    (const char*)p,
                    FMOD_OPENMEMORY | mode,
                    &exinfo,
                    &stream
                );
                FMOD_ERROR_CHECK(result);

                if (result != FMOD_OK) {
                    result = FMOD_System_CreateSound(
                        sound.fmod_studio_system_music,
                        (const char*)p,
                        FMOD_OPENMEMORY | FMOD_CREATECOMPRESSEDSAMPLE | mode,
                        &exinfo,
                        &stream
                    );
                    FMOD_ERROR_CHECK(result);
                }
            }

            if (result == FMOD_OK && stream) {
                currentMidiSound = stream;
                result = FMOD_System_PlaySound(sound.fmod_studio_system_music, stream, sound.master_music, false, &sound.fmod_studio_channel_music);
                FMOD_ERROR_CHECK(result);
                if (result == FMOD_OK) {
                    FMOD_ERROR_CHECK(FMOD_Channel_SetVolume(sound.fmod_studio_channel_music, final_volume));
                    FMOD_ERROR_CHECK(FMOD_Channel_SetVolumeRamp(sound.fmod_studio_channel_music, false));
                    return mus_id;
                }
                CON_Warnf("FMOD_StartMusic: Play failed for lump %d.\n", mus_id);
                ReleaseSound(&currentMidiSound);
                StopChannel(&sound.fmod_studio_channel_music);
                return -1;
            }

            CON_Warnf("FMOD_StartMusic: Failed to create stream for lump %d. Result: %d\n", mus_id, result);
            ReleaseSound(&currentMidiSound);
            return -1;
        }
    }

    CON_Warnf("FMOD_StartMusic: mus_id %d is out of bounds or invalid.\n", mus_id);
    return -1;
}

void FMOD_StopMusic(sndsrc_t* origin, int mus_id) {
    StopChannel(&sound.fmod_studio_channel_music);
}

void FMOD_PauseMusic(void) {
    if(sound.fmod_studio_channel_music) {
        FMOD_ERROR_CHECK(FMOD_Channel_SetPaused(sound.fmod_studio_channel_music, true));
    }
}

void FMOD_ResumeMusic(void) {
    if(sound.fmod_studio_channel_music) {
        FMOD_ERROR_CHECK(FMOD_Channel_SetPaused(sound.fmod_studio_channel_music, false));
    }
}

void FMOD_PauseSFXLoop(void) {
    if(sound.fmod_studio_channel_loop) {
        FMOD_ERROR_CHECK(FMOD_Channel_SetPaused(sound.fmod_studio_channel_loop, true));
    }
}

void FMOD_ResumeSFXLoop(void) {
    if(sound.fmod_studio_channel_loop) {
        FMOD_ERROR_CHECK(FMOD_Channel_SetPaused(sound.fmod_studio_channel_loop, false));
    }
}
