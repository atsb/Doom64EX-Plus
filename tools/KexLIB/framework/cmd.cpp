// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2012 Samuel Villarreal
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
// DESCRIPTION: Command System
//
//-----------------------------------------------------------------------------

#include "kexlib.h"

static kexCommand command;
kexCommand *kexlib::commands = &command;

//
// kexCommandItem::kexCommandItem
//

kexCommandItem::kexCommandItem(const char *commandName, cmd_t commandFunc) {
    Setup(commandName, commandFunc);
}

//
// kexCommandItem::Setup
//

void kexCommandItem::Setup(const char *commandName, cmd_t commandFunc) {
    this->name          = commandName;
    this->function      = commandFunc;
    this->next          = NULL;
    this->bAllocated    = false;

    command.RegisterCommand(this);
}

//
// kexCommand::~kexCommand
//

kexCommand::~kexCommand(void) {
    for(kexCommandItem *cmd = first; cmd;) {
        kexCommandItem *free;

        if(cmd->IsAllocated()) {
            free = cmd;
            cmd = free->GetNext();
            delete free;
        }
        else {
            cmd = cmd->GetNext();
        }
    }
}

//
// kexCommand::RegisterCommand
//

void kexCommand::RegisterCommand(kexCommandItem *commandItem) {
    if(first == NULL) {
        first = commandItem;
    }

    if(next != NULL) {
        next->SetNext(commandItem);
    }

    next = commandItem;
}

//
// kexCommand::Run
//

bool kexCommand::Run(void) {
    kexCommandItem *cmd;

    if(!cmd_argc) {
        return false;
    }

    for(cmd = first; cmd; cmd = cmd->GetNext()) {
        if(!kexStr::Compare(cmd_argv[0], cmd->GetName())) {
            if(cmd->function) {
                cmd->function();
            }
            return true;
        }
    }

    return false;
}

//
// kexCommand::ClearArgv
//

void kexCommand::ClearArgv(void) {
    memset(cmd_argv, 0, CMD_MAX_ARGV * CMD_BUFFER_LEN);
}

//
// kexCommand::GetArgc
//

int kexCommand::GetArgc(void) {
    return cmd_argc;
}

//
// kexCommand::GetArgv
//

char *kexCommand::GetArgv(int argv) {
    return cmd_argv[argv];
}

//
// kexCommand::Execute
//

void kexCommand::Execute(const char *buffer) {
    int len;
    int j;
    char *b_rover;
    char *a_rover;
    bool havetoken;
    bool inquotes;

    j = 0;
    cmd_argc = 0;
    ClearArgv();
    len = strlen(buffer);
    b_rover = (char*)buffer;
    havetoken = false;
    inquotes = false;

    while(1) {
        if(b_rover - buffer > len) {
            // end of buffer
            return;
        }

        a_rover = cmd_argv[cmd_argc];

        // skip spaces
        if(*b_rover == ' ') {
            while(*b_rover == ' ') {
                b_rover++;
            }
        }

        havetoken = false;

        // search for a token
        while(*b_rover != ' ' || inquotes) {
            if(*b_rover == '"') {
                if(inquotes) {
                    inquotes = false;
                }
                else {
                    inquotes = true;
                }

                b_rover++;
                continue;
            }

            // execute commands after a newline or semicolon
            if(*b_rover == '\n' || (*b_rover == ';' && !inquotes) || (*b_rover == 0 && havetoken)) {
                if(inquotes) {
                    kexlib::system->Warning("Command contains incomplete quote\n");
                    return;
                }

                if(!Run()) {
                    kexCvar *cvar;

                    // check to see if command maches a cvar
                    if((cvar = kexlib::cvars->Get(cmd_argv[0]))) {
                        if(cmd_argc == 1) {
                            kexlib::system->Printf("%s: %s (%s)\n", cvar->GetName(),
                                cvar->GetValue(), cvar->GetDefaultValue());
                        }
                        else {
                            kexlib::cvars->Set(cvar->GetName(), cmd_argv[1]);
                        }
                    }
                    else {
                        // no match, assume typo
                        kexlib::system->Warning("Unknown command: %s\n", cmd_argv[0]);
                    }
                }

                cmd_argc = 0;
                j = 0;
                havetoken = false;
                memset(cmd_argv, 0, CMD_MAX_ARGV*CMD_BUFFER_LEN);
                a_rover = cmd_argv[cmd_argc];
                b_rover++;
            }
            else if(*b_rover < ' ') {
                b_rover++;
                continue;
            }
            else {
                *(a_rover++) = *(b_rover++);

                // a token as been found; increment argc
                if(!havetoken) {
                    if(++cmd_argc >= CMD_MAX_ARGV) {
                        break;
                    }

                    havetoken = true;
                }

                if(++j >= CMD_BUFFER_LEN) {
                    kexlib::system->Warning("Command string is too long (%i limit)\n",
                        CMD_BUFFER_LEN);
                    return;
                }
            }

            if(b_rover - buffer > len) {
                // end of buffer
                return;
            }
        }
    }
}

//
// kexCommand::AutoComplete
//

bool kexCommand::AutoComplete(const char *partial) {
    int len = strlen(partial);
	
    if(!len) {
        return false;
    }

    bool ok = false;
    
    // check for exact match
    for(kexCommandItem *cmd = first; cmd; cmd = cmd->GetNext()) {
        if(!strcmp(partial, cmd->GetName())) {
            if(!ok) {
                kexlib::system->CPrintf(COLOR_CYAN, "\nCommands:\n");
                ok = true;
            }

            kexlib::system->CPrintf(COLOR_GREEN, "%s\n", cmd->GetName());
        }
    }

    // check for partial match
    for(kexCommandItem *cmd = first; cmd; cmd = cmd->GetNext()) {
        if(!strncmp(partial, cmd->GetName(), len)) {
            if(!ok) {
                kexlib::system->CPrintf(COLOR_CYAN, "\nCommands:\n");
                ok = true;
            }

            kexlib::system->CPrintf(COLOR_GREEN, "%s\n", cmd->GetName());
        }
    }
    
    return ok;
}

//
// kexCommand::Verify
//

bool kexCommand::Verify(const char *name) {
    kexCommandItem *cmd;

    // fail if the command is a variable name
    if(kexlib::cvars->Get(name)) {
        kexlib::system->Warning("Cmd_AddCommand: %s already defined as a var\n", name);
        return false;
    }
    
    // fail if the command already exists
    for(cmd = first; cmd; cmd = cmd->GetNext()) {
        if(!strcmp(name, cmd->GetName())) {
            kexlib::system->Warning("Cmd_AddCommand: %s already defined\n", name);
            return false;
        }
    }

    return true;
}

//
// kexCommand::Add
//

void kexCommand::Add(const char *name, cmd_t function) {
	kexCommandItem *cmd;
	
    if(!Verify(name)) {
        return;
    }

    cmd = new kexCommandItem(name, function);
    cmd->bAllocated = true;
}

//
// kexCommand::GetFunctions
//

kexCommandItem *kexCommand::GetFunctions(void) {
    return first;
}

//
// listcmds
//

COMMAND(listcmds) {
    kexCommandItem *cmd;
    int i = 0;

    for(cmd = command.GetFunctions(); cmd; cmd = cmd->GetNext(), i++) {
        kexlib::system->CPrintf(COLOR_CYAN, "%s\n", cmd->GetName());
    }
    
    kexlib::system->CPrintf(COLOR_GREEN, "%i commands\n", i);
}
