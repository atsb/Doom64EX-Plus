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

#include <fmod.h>
#include <fmod_errors.h>

#include "doomtype.h"
#include "doomdef.h"
#include "i_system.h"
#include "i_audio.h"
#include "w_wad.h"
#include "z_zone.h"
#include "i_swap.h"
#include "con_console.h"    // for cvars
#include "d_player.h"

// 20120203 villsa - cvar for soundfont location
CVAR(s_soundfont, doomsnd.sf2);
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

// FMOD Studio
static float INCHES_PER_METER = 39.3701f;
int num_sfx;

FMOD_REVERB_PROPERTIES reverb_prop = FMOD_PRESET_GENERIC;

FMOD_VECTOR fmod_reverb_position = { -8.0f, 2.0f, 2.0f };
float min_dist = 1.0f;
float max_dist = 15.0f;

FMOD_BOOL IsPlaying;
FMOD_BOOL Paused = FALSE;

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

//
// Signal_UpdateGain
//
static int Signal_UpdateGain(float db) {

}

static const signalhandler seqsignallist[MAXSIGNALTYPES] = {
    Signal_Idle,
    Signal_Shutdown,
    NULL,
    Signal_Reset,
    Signal_Pause,
    Signal_Resume,
    Signal_StopAll,
    Signal_UpdateGain
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
// Seq_Shutdown
//

static void Seq_Shutdown(doomseq_t* seq) {
    //
    // signal the sequencer to shut down
    //
    Seq_SetStatus(seq, SEQ_SIGNAL_SHUTDOWN);

#ifdef _WIN32
    //
    // Screw the shutdown, the OS will handle it :P
    //
#else
    //
    // wait until the audio thread is finished
    //
    SDL_WaitThread(seq->thread, NULL);

    //
    // fluidsynth cleanup stuff
    //
    delete_fluid_audio_driver(seq->driver);
    delete_fluid_synth(seq->synth);
    delete_fluid_settings(seq->settings);

    seq->synth = NULL;
    seq->driver = NULL;
    seq->settings = NULL;
#endif
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

    I_Printf("\n--------Initializing FMOD Studio--------\n");
    I_Printf("Made with FMOD Studio by Firelight Technologies Pty Ltd.\n\n");

    FMOD_ERROR_CHECK(FMOD_System_SetDSPBufferSize(sound.fmod_studio_system, 1024, 128));
    FMOD_ERROR_CHECK(FMOD_System_SetDSPBufferSize(sound.fmod_studio_system_music, 1024, 128));

    FMOD_ERROR_CHECK(FMOD_System_Create(&sound.fmod_studio_system, FMOD_VERSION));
    FMOD_ERROR_CHECK(FMOD_System_Create(&sound.fmod_studio_system_music, FMOD_VERSION));

    FMOD_ERROR_CHECK(FMOD_System_Init(sound.fmod_studio_system, 92, FMOD_INIT_3D_RIGHTHANDED | FMOD_INIT_PROFILE_ENABLE, NULL));
    FMOD_ERROR_CHECK(FMOD_System_Init(sound.fmod_studio_system_music, 128, FMOD_INIT_NORMAL, NULL));

    // FMOD_ERROR_CHECK(FMOD_System_CreateReverb3D(sound.fmod_studio_system, reverb.fmod_reverb));

    FMOD_ERROR_CHECK(FMOD_System_GetMasterChannelGroup(sound.fmod_studio_system, &sound.master));
    FMOD_ERROR_CHECK(FMOD_System_GetMasterChannelGroup(sound.fmod_studio_system_music, &sound.master_music));

    // Set 3D min/max distance for each sound source if needed
    FMOD_ERROR_CHECK(FMOD_Sound_Set3DMinMaxDistance(sound.fmod_studio_sound[num_sfx], 0.5f * INCHES_PER_METER, 127.0f * INCHES_PER_METER));
    // Add similar lines for other sound sources if necessary

    // Setup external tracks

    FMOD_CreateMusicTracksInit();

    FMOD_CreateSfxTracksInit();

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

// FMOD Studio SFX API

int FMOD_StartSound(int sfx_id, sndsrc_t* origin, int volume, int pan, float properties_reverb) {

    FMOD_System_SetReverbProperties(sound.fmod_studio_system, (int)properties_reverb, &reverb_prop);

    FMOD_ERROR_CHECK(FMOD_Channel_SetPaused(sound.fmod_studio_channel, false));
    FMOD_ERROR_CHECK(FMOD_System_PlaySound(sound.fmod_studio_system, sound.fmod_studio_sound[sfx_id], sound.master, 0, &sound.fmod_studio_channel));
    
    FMOD_Channel_SetPaused(sound.fmod_studio_channel_loop, false);

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

int FMOD_StartSFXLoop(int sfx_id) {
    FMOD_Channel_SetVolume(sound.fmod_studio_channel_loop, 20.0f);
    FMOD_ERROR_CHECK(FMOD_System_PlaySound(sound.fmod_studio_system, sound.fmod_studio_sound[sfx_id], sound.master, 0, &sound.fmod_studio_channel_loop));

    FMOD_ERROR_CHECK(FMOD_Channel_SetVolumeRamp(sound.fmod_studio_channel, false));

    FMOD_Channel_SetPaused(sound.fmod_studio_channel_loop, false);

    return sfx_id;
}

int FMOD_StopSFXLoop(void) {
    FMOD_ERROR_CHECK(FMOD_Channel_Stop(sound.fmod_studio_channel_loop));

    FMOD_Channel_SetPaused(sound.fmod_studio_channel_loop, true);

    return 0;
}

void FMOD_StopSound(sndsrc_t* origin, int sfx_id) {
    FMOD_ERROR_CHECK(FMOD_Channel_Stop(sound.fmod_studio_channel));

    FMOD_Channel_SetPaused(sound.fmod_studio_channel, true);
}

int FMOD_StartMusic(int mus_id) {
    FMOD_RESULT result;
    FMOD_BOOL isPlaying = false;
    const int MAX_FMOD_MUSIC_TRACKS = 138; // Size of sound.fmod_studio_music[]

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

    // Check if mus_id (music_id) is intended for a MIDI track
    if (mus_id >= 0 && mus_id < doomseq.nsongs && doomseq.songs[mus_id].data != NULL) {
        FMOD_CREATESOUNDEXINFO exinfo;
        dmemset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO)); // setting the memory size 
        exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO); // size
        exinfo.length = doomseq.songs[mus_id].length; // length of the music

        // Hardcode "DOOMSND.DLS" for MIDI playback, we actually NEED IT!!
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
                false, // Not paused
                &sound.fmod_studio_channel_music);
            FMOD_ERROR_CHECK(result);
            if (result == FMOD_OK) {
                FMOD_ERROR_CHECK(FMOD_Channel_SetVolumeRamp(sound.fmod_studio_channel_music, false));
                return mus_id;
            }
            else {
                CON_Printf("FMOD_StartMusic: Failed to play MIDI sound for mus_id %d after creation.\n", mus_id);
                FMOD_ERROR_CHECK(FMOD_Sound_Release(currentMidiSound));
                currentMidiSound = NULL;
                return -1;
            }
        }
        else {
            CON_Printf("FMOD_StartMusic: Failed to create MIDI sound for mus_id %d.\n", mus_id);
            if (currentMidiSound) {
                FMOD_ERROR_CHECK(FMOD_Sound_Release(currentMidiSound));
                currentMidiSound = NULL;
            }
            return -1;
        }
    }
    // mus_id is intended for a pre-rendered track
    else if (mus_id >= 0 && mus_id < MAX_FMOD_MUSIC_TRACKS) {
        if (sound.fmod_studio_music[mus_id] != NULL) {
            FMOD_ERROR_CHECK(FMOD_System_PlaySound(sound.fmod_studio_system_music, sound.fmod_studio_music[mus_id], sound.master_music, 0, &sound.fmod_studio_channel_music));
            FMOD_ERROR_CHECK(FMOD_Channel_SetVolumeRamp(sound.fmod_studio_channel_music, false));
            FMOD_Channel_SetPaused(sound.fmod_studio_channel_music, false);
            return mus_id;
        }
        else {
            CON_Printf("FMOD_StartMusic: Pre-rendered track for mus_id %d is NULL.\n", mus_id);
            return -1;
        }
    }
    // mus_id is out of bounds
    else {
        CON_Printf("FMOD_StartMusic: mus_id %d is out of bounds for MIDI and pre-rendered tracks.\n", mus_id);
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
