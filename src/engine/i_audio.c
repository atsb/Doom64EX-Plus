// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2007-2012 Samuel Villarreal
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
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

#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef __OpenBSD__
#include <SDL3/SDL.h>
#else
#include <SDL3/SDL.h>
#endif

#include "doomtype.h"
#include "doomdef.h"
#include "doomstat.h"
#include "i_system.h"
#include "i_audio.h"
#include "w_wad.h"
#include "z_zone.h"
#include "i_swap.h"
#include "con_console.h"
#include "d_player.h"
#include "tables.h"

// Thresholds for distance checks in game units
static const float GAME_SFX_MAX_UNITS_THRESHOLD = 1200.0f;
static const float GAME_SFX_FORCED_FULL_VOL_UNITS_THRESHOLD = 400.0f;

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

// 20120203 villsa - cvar for audio driver
#ifdef _WIN32
CVAR_CMD(s_driver, dsound)
#elif __linux__
CVAR_CMD(s_driver, alsa)
#elif __APPLE__
CVAR_CMD(s_driver, coreaudio)
#else
CVAR_CMD(s_driver, sndio)
#endif
{
    char* driver = cvar->string;

    if (!dstrcmp(driver, "jack") ||
        !dstrcmp(driver, "alsa") ||
        !dstrcmp(driver, "oss") ||
        !dstrcmp(driver, "pulseaudio") ||
        !dstrcmp(driver, "coreaudio") ||
        !dstrcmp(driver, "dsound") ||
        !dstrcmp(driver, "portaudio") ||
        !dstrcmp(driver, "sndio") ||
        !dstrcmp(driver, "sndman") ||
        !dstrcmp(driver, "dart") ||
        !dstrcmp(driver, "file")
        ) {
        return;
    }

    CON_Warnf("Invalid driver name\n");
    CON_Warnf("Valid driver names: jack, alsa, oss, pulseaudio, coreaudio, dsound, portaudio, sndio, sndman, dart, file\n");
    CON_CvarSet(cvar->name, DEFAULT_FLUID_DRIVER);
}

struct Sound sound;
struct Reverb fmod_reverb;

#define MAX_GAME_SFX 256

// FMOD Studio
static float INCHES_PER_METER = 39.3701f;
int num_sfx;

FMOD_REVERB_PROPERTIES reverb_prop = FMOD_PRESET_GENERIC;

FMOD_VECTOR fmod_reverb_position = { -8.0f, 2.0f, 2.0f };
float min_dist = 1.0f;
float max_dist = 15.0f;

FMOD_BOOL IsPlaying;
FMOD_BOOL Paused = false;

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
        printf("FMOD Studio: %s", FMOD_ErrorString(result));
    }
}

//
// Seq_SetGain
//
// Set the 'master' volume for the sequencer. Affects
// all sounds that are played
//

void Seq_SetGain(float db) {
    FMOD_Channel_SetLowPassGain(sound.fmod_studio_channel, db);
    FMOD_Channel_SetLowPassGain(sound.fmod_studio_channel_loop, db);
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
// Chan_StopTrack
//
// Stops a specific channel and any played sounds
//

static void Chan_StopTrack(doomseq_t* seq, channel_t* chan) {
    int c;

    if (chan->song->type >= 1) {
        c = chan->track->channel;
    }
    else {
        c = chan->id;
    }
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

    Chan_StopTrack(seq, chan);

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
// Song_AddTrackToPlaylist
//
// Add a song to the playlist for the sequencer to play.
// Sets any default values to the channel in the process
// Both start sound and start music refer to this

static channel_t* Song_AddTrackToPlaylist(doomseq_t* seq, song_t* song, track_t* track) {
    int i;

    for (i = 0; i < MIDI_CHANNELS; i++) {

        if (playlist[i].song == NULL) {
            playlist[i].song = song;
            playlist[i].track = track;
            playlist[i].tics = 0;
            playlist[i].lasttic = 0;
            playlist[i].starttic = 0;
            playlist[i].pos = track->data;
            playlist[i].jump = NULL;
            playlist[i].state = CHAN_STATE_READY;
            playlist[i].paused = false;
            playlist[i].stop = false;
            playlist[i].key = 0;
            playlist[i].velocity = 0;

            // channels 0 through 15 are reserved for music only
            // channel ids should only be accessed by non-music sounds
            playlist[i].id = 0x0f + i;

            playlist[i].volume = 127.0f;
            playlist[i].basevol = 127.0f;
            playlist[i].pan = 64;
            playlist[i].origin = NULL;
            playlist[i].depth = 0;
            playlist[i].starttime = 0;
            playlist[i].curtime = 0;

            // immediately start reading the midi track
            playlist[i].nexttic = Chan_GetNextTick(&playlist[i]);

            seq->voices++;

            return &playlist[i];
        }
    }

    return NULL;
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
    int program;

    program = Chan_GetNextMidiByte(chan);
}

//
// Event_ChannelPressure
//

static void Event_ChannelPressure(doomseq_t* seq, channel_t* chan) {
    int val;

    val = Chan_GetNextMidiByte(chan);
}

//
// Event_PitchBend
//

static void Event_PitchBend(doomseq_t* seq, channel_t* chan) {
    int b1;
    int b2;

    b1 = Chan_GetNextMidiByte(chan);
    b2 = Chan_GetNextMidiByte(chan);
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
    int i;
    channel_t* c;

    for (i = 0; i < MIDI_CHANNELS; i++) {
        c = &playlist[i];
    }

    Seq_SetStatus(seq, SEQ_SIGNAL_READY);
    return 1;
}

//
// Signal_Resume
//
// Resume all songs that were paused
//

static int Signal_Resume(doomseq_t* seq) {
    int i;
    channel_t* c;

    for (i = 0; i < MIDI_CHANNELS; i++) {
        c = &playlist[i];
    }

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
    int i;
    int start;
    int end;
    int fail;

    seq->nsongs = 0;
    i = 0;

    start = W_GetNumForName("DM_START") + 1;
    end = W_GetNumForName("DM_END") - 1;

    seq->nsongs = (end - start) + 1;

    //
    // no midi songs found in iwad?
    //
    if (seq->nsongs <= 0) {
        return false;
    }

    seq->songs = (song_t*)Z_Calloc(seq->nsongs * sizeof(song_t), PU_STATIC, 0);

    fail = 0;
    for (i = 0; i < seq->nsongs; i++) {
        song_t* song;

        song = &seq->songs[i];
        song->data = W_CacheLumpNum(start + i, PU_STATIC);
        song->length = W_LumpLength(start + i);

        if (!song->length) {
            continue;
        }

        dmemcpy(song, song->data, 0x0e);
        if (dstrncmp(song->header, "MThd", 4)) {
            fail++;
            continue;
        }

        song->chunksize = I_SwapBE32(song->chunksize);
        song->ntracks = I_SwapBE16(song->ntracks);
        song->delta = I_SwapBE16(song->delta);
        song->type = I_SwapBE16(song->type);
        song->timediv = Song_GetTimeDivision(song);
        song->tempo = 480000;

        if (!Song_RegisterTracks(song)) {
            fail++;
            continue;
        }
    }

    if (fail) {
        I_Printf("Failed to load %d MIDI tracks.\n", fail);
    }

    return true;
}

//
// Seq_RegisterSounds
//
// Load sound effects from WAD lumps (between DS_START and DS_END)
//

static int Seq_RegisterSounds(void) {
    int i;
    int start, end;
    int first_sfx_lump_index, num_sfx_lumps_to_process;
    int success = 0;
    int fail = 0;
    FMOD_RESULT result;

    // Initialize FMOD_SOUND array
    for (i = 0; i < MAX_GAME_SFX; ++i) {
        sound.fmod_studio_sound[i] = NULL;
    }
    num_sfx = 0;

    start = W_GetNumForName("DS_START") + 1;
    end = W_GetNumForName("DS_END") - 1;

    num_sfx_lumps_to_process = (end - start) + 1;

    if (num_sfx_lumps_to_process <= 0) {
        I_Printf("Seq_RegisterSounds: No sound effect lumps found between DS_START and DS_END.\n");
        return true;
    }

    for (i = 0; i < num_sfx_lumps_to_process; i++) {
        if (num_sfx >= MAX_GAME_SFX) {
            CON_Warnf("Seq_RegisterSounds: Exceeded limit of %d. Further SFX ignored.\n", MAX_GAME_SFX);
            fail += (num_sfx_lumps_to_process - i);
            break;
        }

        int current_lump_num = start + i;
        int sfx_lump_length = W_LumpLength(current_lump_num);
        void* sfx_lump_data = W_CacheLumpNum(current_lump_num, PU_STATIC);
        if (!sfx_lump_data) {
            sound.fmod_studio_sound[num_sfx++] = NULL;
            fail++;
            continue;
        }

        FMOD_CREATESOUNDEXINFO exinfo;
        dmemset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
        exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
        exinfo.length = sfx_lump_length;


        result = FMOD_System_CreateSound(
            sound.fmod_studio_system,
            (const char*)sfx_lump_data,
            FMOD_OPENMEMORY_POINT | FMOD_3D | FMOD_LOOP_OFF,
            &exinfo,
            &sound.fmod_studio_sound[num_sfx]
        );

        if (result == FMOD_OK && sound.fmod_studio_sound[num_sfx]) {
            float sfx_min_dist_scaled = GAME_SFX_FORCED_FULL_VOL_UNITS_THRESHOLD * INCHES_PER_METER;
            float sfx_max_dist_scaled = GAME_SFX_MAX_UNITS_THRESHOLD * INCHES_PER_METER;
            FMOD_Sound_Set3DMinMaxDistance(sound.fmod_studio_sound[num_sfx], sfx_min_dist_scaled, sfx_max_dist_scaled);
            success++;
        }
        else {
            sound.fmod_studio_sound[num_sfx] = NULL;
            fail++;
        }
        num_sfx++;
    }

    if (fail > 0) {
        I_Printf("Failed to load %d sound effects.\n", fail);
    }
    if (success > 0) {
        I_Printf("Successfully registered %d sound effects.\n", success);
    }

    return true;
}

//
// Seq_Shutdown
//

static void Seq_Shutdown(doomseq_t* seq) {
    //
    // signal the sequencer to shut down
    //
    Seq_SetStatus(seq, SEQ_SIGNAL_SHUTDOWN);
    SDL_WaitThread(seq->thread, NULL);
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
    int   sffound;
    char* sfpath;
    void* extradriverdata = 0;

    I_Printf("Audio Engine: FMOD Studio by Firelight Technologies Pty Ltd.\n\n");

    FMOD_ERROR_CHECK(FMOD_System_SetDSPBufferSize(sound.fmod_studio_system, 1024, 128));
    FMOD_ERROR_CHECK(FMOD_System_SetDSPBufferSize(sound.fmod_studio_system_music, 1024, 128));

    FMOD_ERROR_CHECK(FMOD_System_Create(&sound.fmod_studio_system, FMOD_VERSION));
    FMOD_ERROR_CHECK(FMOD_System_Create(&sound.fmod_studio_system_music, FMOD_VERSION));

    FMOD_ERROR_CHECK(FMOD_System_Init(sound.fmod_studio_system, 92, FMOD_INIT_3D_RIGHTHANDED | FMOD_INIT_PROFILE_ENABLE, NULL));
    FMOD_ERROR_CHECK(FMOD_System_Init(sound.fmod_studio_system_music, 128, FMOD_INIT_NORMAL, NULL));

    FMOD_ERROR_CHECK(FMOD_System_GetMasterChannelGroup(sound.fmod_studio_system, &sound.master));
    FMOD_ERROR_CHECK(FMOD_System_GetMasterChannelGroup(sound.fmod_studio_system_music, &sound.master_music));

    // Set 3D min/max distance for each sound source if needed
    FMOD_ERROR_CHECK(FMOD_Sound_Set3DMinMaxDistance(sound.fmod_studio_sound[num_sfx], 0.5f * INCHES_PER_METER, 127.0f * INCHES_PER_METER));
    // Add similar lines for other sound sources if necessary

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

    FMOD_VECTOR soundPosition = { x, y, 0.0f };  // Set the appropriate position in 3D space
    FMOD_Channel_Set3DAttributes(sound.fmod_studio_channel, &soundPosition, NULL);

    FMOD_Channel_SetMode(sound.fmod_studio_channel, FMOD_3D_WORLDRELATIVE);

    chan = &playlist[c];
    chan->basevol = (float)volume;
    chan->pan = (byte)(pan >> 1);
}

//
// I_ShutdownSound
// Shutdown sound when player exits the game / error occurs

void I_ShutdownSound(void)
{
    FMOD_ERROR_CHECK(FMOD_System_Close(sound.fmod_studio_system));
    FMOD_ERROR_CHECK(FMOD_System_Release(sound.fmod_studio_system));

    FMOD_ERROR_CHECK(FMOD_System_Close(sound.fmod_studio_system_music));
    FMOD_ERROR_CHECK(FMOD_System_Release(sound.fmod_studio_system_music));
}

//
// I_SetMusicVolume
//

void I_SetMusicVolume(float volume) {
    FMOD_ERROR_CHECK(FMOD_Channel_SetVolume(sound.fmod_studio_channel_music, volume / 255.0f));
    doomseq.musicvolume = (volume * 0.925f);
}

//
// I_SetSoundVolume
//

void I_SetSoundVolume(float volume) {
    FMOD_ERROR_CHECK(FMOD_Channel_SetVolume(sound.fmod_studio_channel, volume / 255.0f));
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

    FMOD_ERROR_CHECK(FMOD_Channel_SetPaused(sound.fmod_studio_channel, true));
    Seq_SetStatus(&doomseq, SEQ_SIGNAL_PAUSE);
}

//
// I_ResumeSound
//

void I_ResumeSound(void) {
    if (!seqready) {
        return;
    }

    FMOD_ERROR_CHECK(FMOD_Channel_SetPaused(sound.fmod_studio_channel, false));
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

    FMOD_Channel_SetLowPassGain(sound.fmod_studio_channel, db);
    FMOD_Channel_SetLowPassGain(sound.fmod_studio_channel_loop, db);
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

    listener_forward.x = (float)finecosine[view_angle >> ANGLETOFINESHIFT] / FRACUNIT;
    listener_forward.z = (float)finesine[view_angle >> ANGLETOFINESHIFT] / FRACUNIT;
    listener_forward.y = 0.0f;

    FMOD_System_Set3DListenerAttributes(sound.fmod_studio_system, 0, &listener_pos, &listener_vel, &listener_forward, &listener_up);
    FMOD_System_Update(sound.fmod_studio_system);
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

    if (sfx_channel) {
        float instance_vol = (float)volume / 255.0f;
        float global_sfx_vol = (s_sfxvol.value >= 0 && s_sfxvol.value <= 255) ? (float)s_sfxvol.value / 255.0f : 1.0f;
        float final_volume = instance_vol * global_sfx_vol;
        if (isnan(final_volume) || isinf(final_volume)) final_volume = 0.75f;
        if (final_volume < 0.0f) final_volume = 0.0f;

        // Clamp to a maximum of 1.0 to prevent clipping (1.0f is really loud for FMOD)
        if (final_volume > 1.0f) {
            final_volume = 1.0f;
        }
        FMOD_Channel_SetVolume(sfx_channel, final_volume);

        if (origin) {
            mobj_t* mobj = (mobj_t*)origin;
            FMOD_VECTOR pos, vel;

            pos.x = (float)mobj->x / FRACUNIT * INCHES_PER_METER;
            pos.y = (float)mobj->z / FRACUNIT * INCHES_PER_METER;
            pos.z = (float)mobj->y / FRACUNIT * INCHES_PER_METER;

            vel.x = 0.0f; vel.y = 0.0f; vel.z = 0.0f;

            FMOD_Channel_Set3DAttributes(sfx_channel, &pos, &vel);
            FMOD_Channel_SetMode(sfx_channel, FMOD_3D_WORLDRELATIVE);
        }
        else {
            float fmod_pan = ((float)pan - 128.0f) / 128.0f;
            if (isnan(fmod_pan) || isinf(fmod_pan)) fmod_pan = 0.0f;
            fmod_pan = (fmod_pan < -1.0f) ? -1.0f : (fmod_pan > 1.0f) ? 1.0f : fmod_pan;
            FMOD_Channel_SetPan(sfx_channel, fmod_pan);
            FMOD_Channel_SetMode(sfx_channel, FMOD_2D);
        }
        FMOD_Channel_SetPaused(sfx_channel, false);
    }
    else {
        CON_Warnf("FMOD_StartSound: PlaySound OK (default path) but channel is NULL (sfx_id %d)\n", sfx_id);
        return -1;
    }

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
                FMOD_Channel_SetVolume(sfx_channel, final_volume);
                FMOD_Channel_SetPan(sfx_channel, 0.0f);
                FMOD_Channel_SetMode(sfx_channel, FMOD_2D);
                FMOD_Channel_SetPaused(sfx_channel, false);
                return sfx_id;
            }
            else {
                CON_Warnf("FMOD_StartSound: PlaySound OK (2D override) but channel is NULL (sfx_id %d)\n", sfx_id);
                return -1;
            }
        }
    }
    return sfx_id;
}

// Not proud of it here but it is a necessary evil for now, to prevent cut-off between plasma fire and plasma ball boom
int FMOD_StartSoundPlasma(int sfx_id) {
    FMOD_ERROR_CHECK(FMOD_System_PlaySound(sound.fmod_studio_system, sound.fmod_studio_sound_plasma[sfx_id], sound.master, 0, &sound.fmod_studio_channel));

    FMOD_ERROR_CHECK(FMOD_Channel_SetVolumeRamp(sound.fmod_studio_channel, false));
    FMOD_ERROR_CHECK(FMOD_Channel_SetPaused(sound.fmod_studio_channel, false));

    FMOD_Channel_SetPaused(sound.fmod_studio_channel_loop, false);

    return sfx_id;
}

int FMOD_StartSFXLoop(int sfx_id, int volume) {
    if (sound.fmod_studio_channel_loop) {
        FMOD_Channel_Stop(sound.fmod_studio_channel_loop);
    }

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
        FMOD_Channel_SetVolume(sound.fmod_studio_channel_loop, final_volume);
        FMOD_Channel_SetMode(sound.fmod_studio_channel_loop, FMOD_LOOP_NORMAL | FMOD_2D);
        FMOD_Channel_SetVolumeRamp(sound.fmod_studio_channel_loop, false);
        FMOD_Channel_SetPaused(sound.fmod_studio_channel_loop, false); // Unpause
    }
    else {
        CON_Warnf("FMOD_StartSFXLoop: Failed to play sound or get channel (sfx_id %d)\n", sfx_id);
        return -1;
    }
    return sfx_id;
}

void FMOD_StopPlasmaLoop(void) {
    if (sound.fmod_studio_channel_plasma_loop) {
        FMOD_Channel_IsPlaying(sound.fmod_studio_channel_plasma_loop, &IsPlaying);
        if (IsPlaying) {
            FMOD_Channel_Stop(sound.fmod_studio_channel_plasma_loop);
        }
        sound.fmod_studio_channel_plasma_loop = NULL;
    }
}

int FMOD_StopSFXLoop(void) {
    FMOD_ERROR_CHECK(FMOD_Channel_Stop(sound.fmod_studio_channel_loop));

    FMOD_Channel_SetPaused(sound.fmod_studio_channel_loop, true);

    return 0;
}

int FMOD_StartPlasmaLoop(int sfx_id, int volume) {
    if (sound.fmod_studio_channel_plasma_loop) {
        FMOD_Channel_IsPlaying(sound.fmod_studio_channel_plasma_loop, &IsPlaying);
        if (IsPlaying) {
            FMOD_Channel_Stop(sound.fmod_studio_channel_plasma_loop);
        }
        sound.fmod_studio_channel_plasma_loop = NULL;
    }

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
        FMOD_Channel_SetVolume(sound.fmod_studio_channel_plasma_loop, 0.1f);
        FMOD_Channel_SetMode(sound.fmod_studio_channel_plasma_loop, FMOD_LOOP_NORMAL | FMOD_2D);
        FMOD_Channel_SetVolumeRamp(sound.fmod_studio_channel_plasma_loop, false);
        FMOD_Channel_SetPaused(sound.fmod_studio_channel_plasma_loop, false);
    }
    else {
        CON_Warnf("FMOD_StartPlasmaLoop: Failed to play sound or get channel (sfx_id %d)\n", sfx_id);
        sound.fmod_studio_channel_plasma_loop = NULL;
        return -1;
    }
    return sfx_id;
}

void FMOD_StopSound(sndsrc_t* origin, int sfx_id) {
    FMOD_ERROR_CHECK(FMOD_Channel_Stop(sound.fmod_studio_channel));

    FMOD_Channel_SetPaused(sound.fmod_studio_channel, true);
}

int FMOD_StartMusic(int mus_id) {
    FMOD_RESULT result;
    FMOD_BOOL isPlaying = false;
    const int MAX_FMOD_MUSIC_TRACKS = 138;

    if (sound.fmod_studio_channel_music) {
        FMOD_ERROR_CHECK(FMOD_Channel_IsPlaying(sound.fmod_studio_channel_music, &isPlaying));
        if (isPlaying) {
            FMOD_ERROR_CHECK(FMOD_Channel_Stop(sound.fmod_studio_channel_music));
        }
    }

    if (currentMidiSound) {
        FMOD_ERROR_CHECK(FMOD_Sound_Release(currentMidiSound));
        currentMidiSound = NULL;
    }

    // Prepare volume for the new track
    float final_volume = 0.75f;
    if (s_musvol.value >= 0 && s_musvol.value <= 255) {
        final_volume = (float)s_musvol.value / 255.0f;
    }

    if (isnan(final_volume) || isinf(final_volume)) final_volume = 0.75f;
    if (final_volume < 0.0f) final_volume = 0.0f;
    if (mus_id >= 0 && mus_id < MAX_FMOD_MUSIC_TRACKS && sound.fmod_studio_music[mus_id] != NULL) {
        result = FMOD_System_PlaySound(sound.fmod_studio_system_music,
            sound.fmod_studio_music[mus_id],
            sound.master_music, // Assuming this is the correct channel group
            false,              // Not paused
            &sound.fmod_studio_channel_music);
        FMOD_ERROR_CHECK(result);

        if (result == FMOD_OK) {
            FMOD_ERROR_CHECK(FMOD_Channel_SetVolume(sound.fmod_studio_channel_music, final_volume));
            FMOD_ERROR_CHECK(FMOD_Channel_SetVolumeRamp(sound.fmod_studio_channel_music, false));
            return mus_id;
        }
        else {
            CON_Printf("FMOD_StartMusic: Failed to play pre-rendered track for mus_id %d.\n", mus_id);
            sound.fmod_studio_channel_music = NULL; // channel is null on failure
            return -1;
        }
    }
    else if (mus_id >= 0 && mus_id < doomseq.nsongs && doomseq.songs[mus_id].data != NULL) {
        FMOD_CREATESOUNDEXINFO exinfo;
        dmemset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
        exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
        exinfo.length = doomseq.songs[mus_id].length;
        exinfo.dlsname = "DOOMSND.DLS";

        result = FMOD_System_CreateSound(sound.fmod_studio_system_music,
            (const char*)doomseq.songs[mus_id].data,
            FMOD_OPENMEMORY | FMOD_LOOP_NORMAL | FMOD_2D,
            &exinfo,
            &currentMidiSound);
        FMOD_ERROR_CHECK(result);

        if (result == FMOD_OK && currentMidiSound) {
            result = FMOD_System_PlaySound(sound.fmod_studio_system_music,
                currentMidiSound,
                sound.master_music,
                false,
                &sound.fmod_studio_channel_music);
            FMOD_ERROR_CHECK(result);

            if (result == FMOD_OK) {
                FMOD_ERROR_CHECK(FMOD_Channel_SetVolume(sound.fmod_studio_channel_music, final_volume));
                FMOD_ERROR_CHECK(FMOD_Channel_SetVolumeRamp(sound.fmod_studio_channel_music, false));
                return mus_id;
            }
            else {
                CON_Printf("FMOD_StartMusic: Failed to play MIDI sound for mus_id %d after creation.\n", mus_id);
                FMOD_ERROR_CHECK(FMOD_Sound_Release(currentMidiSound));
                currentMidiSound = NULL;
                sound.fmod_studio_channel_music = NULL;
                return -1;
            }
        }
        else {
            CON_Printf("FMOD_StartMusic: Failed to create MIDI sound for mus_id %d. Result: %d\n", mus_id, result);
            if (currentMidiSound) {
                FMOD_ERROR_CHECK(FMOD_Sound_Release(currentMidiSound));
                currentMidiSound = NULL;
            }
            return -1;
        }
    }
    else {
        if (mus_id >= 0 && mus_id < MAX_FMOD_MUSIC_TRACKS && sound.fmod_studio_music[mus_id] == NULL) {
            CON_Printf("FMOD_StartMusic: Pre-rendered track for mus_id %d is NULL (and not a MIDI).\n", mus_id);
        }
        else if (mus_id >= 0 && mus_id < doomseq.nsongs && doomseq.songs[mus_id].data == NULL) {
            CON_Printf("FMOD_StartMusic: MIDI track data for mus_id %d is NULL (and not pre-rendered).\n", mus_id);
        }
        else {
            CON_Printf("FMOD_StartMusic: mus_id %d is out of bounds or invalid.\n", mus_id);
        }
        return -1;
    }
}

void FMOD_StopMusic(sndsrc_t* origin, int mus_id) {
    FMOD_ERROR_CHECK(FMOD_Channel_Stop(sound.fmod_studio_channel_music));

    FMOD_Channel_SetPaused(sound.fmod_studio_channel_music, true);
}

void FMOD_PauseMusic(void) {
    FMOD_Channel_SetPaused(sound.fmod_studio_channel_music, true);
}

void FMOD_ResumeMusic(void) {
    FMOD_Channel_SetPaused(sound.fmod_studio_channel_music, false);
}

void FMOD_PauseSFXLoop(void) {
    FMOD_Channel_SetPaused(sound.fmod_studio_channel_loop, true);
}

void FMOD_ResumeSFXLoop(void) {
    FMOD_Channel_SetPaused(sound.fmod_studio_channel_loop, false);
}
