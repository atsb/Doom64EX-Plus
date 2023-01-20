// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2014 Samuel Villarreal
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

#ifndef __SYSTEMBASE_H__
#define __SYSTEMBASE_H__

class kexSystem {
public:
                            kexSystem(void);

    virtual void            Main(int argc, char **argv) = 0;
    virtual void            Init(void) = 0;
    virtual void            Sleep(unsigned long usecs);
    virtual void            Shutdown(void) = 0;
    virtual int             GetMS(void);
    virtual uint64_t        GetPerformanceCounter(void);
    virtual int             GetTicks(void);
    virtual void            SpawnInternalConsole(void);
    virtual void            ShowInternalConsole(bool show);
    virtual void            DestroyInternalConsole(void);
    virtual void            SwapBuffers(void) = 0;
    virtual int             GetWindowFlags(void);
    virtual const char      *GetWindowTitle(void);
    virtual void            SetWindowTitle(const char *string);
    virtual void            SetWindowGrab(const bool bEnable);
    virtual void            WarpMouseToCenter(void);
    virtual short           SwapLE16(const short val);
    virtual short           SwapBE16(const short val);
    virtual int             SwapLE32(const int val);
    virtual int             SwapBE32(const int val);
    virtual void            *GetProcAddress(const char *proc);
    virtual int             CheckParam(const char *check);
    virtual const char      *GetBaseDirectory(void);
    virtual void            Log(const char *fmt, ...);
    virtual void            Printf(const char *string, ...);
    virtual void            CPrintf(rcolor color, const char *string, ...);
    virtual void            Warning(const char *string, ...);
    virtual void            DPrintf(const char *string, ...);
    virtual void            Error(const char *string, ...);

    int                     VideoWidth(void) { return videoWidth; }
    int                     VideoHeight(void) { return videoHeight; }
    float                   VideoRatio(void) { return videoRatio; }
    bool                    IsWindowed(void) { return bWindowed; }
    virtual void            *Window(void) { return NULL; }
    bool                    IsShuttingDown(void) { return bShuttingDown; }
    const int               Argc(void) const { return argc; }
    const char              **Argv(void) { return (const char**)argv; }

protected:
    int                     videoWidth;
    int                     videoHeight;
    float                   videoRatio;
    bool                    bWindowed;
    bool                    bShuttingDown;
    FILE                    *f_stdout;
    FILE                    *f_stderr;
    int                     argc;
    char                    **argv;
    char                    *basePath;
};

#endif
