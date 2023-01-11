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

#ifndef __CMD_H__
#define __CMD_H__

//
// COMMANDS
//
typedef void (*cmd_t)(void);

#define COMMAND(name)                                   \
    static void FCmd_ ## name(void);                    \
    kexCommandItem Cmd_ ## name(# name, FCmd_ ## name); \
    static void FCmd_ ## name(void)

class kexCommandItem {
    friend class kexCommand;
public:
                        kexCommandItem(const char *commandName, cmd_t commandFunc);

    kexCommandItem      *GetNext(void) { return next; }
    void                SetNext(kexCommandItem *item) { next = item; }
    const char          *GetName(void) const { return name; }
    const bool          IsAllocated(void) const { return bAllocated; }

private:
    void                Setup(const char *commandName, cmd_t commandFunc);

    const char          *name;
    kexCommandItem      *next;
    cmd_t               function;
    bool                bAllocated;
};

class kexCommand {
    friend class kexCommandItem;
public:
                        ~kexCommand(void);

    int                 GetArgc(void);
    char                *GetArgv(int argv);
    void                Execute(const char *buffer);
    bool                AutoComplete(const char *partial);
    void                Add(const char *name, cmd_t function);
    void                RegisterCommand(kexCommandItem *commandItem);
    kexCommandItem      *GetFunctions(void);
    bool                Verify(const char *name);

private:
    const static int    CMD_MAX_ARGV    = 32;
    const static int    CMD_BUFFER_LEN  = 256;

    void                ClearArgv(void);
    bool                Run(void);

    int                 cmd_argc;
    char                cmd_argv[CMD_MAX_ARGV][CMD_BUFFER_LEN];

    kexCommandItem      *first;
    kexCommandItem      *next;
};

#endif
