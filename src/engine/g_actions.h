// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1999-2000 Paul Brook
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

#ifndef G_ACTIONS_H
#define G_ACTIONS_H

#define MAX_ACTIONPARAM        2

typedef void (*actionproc_t)(int64_t data, char** param);

#define CMD(name) void CMD_ ## name(int64_t data, char** param)

void        G_InitActions(void);
boolean    G_ActionResponder(event_t* ev);
void        G_AddCommand(char* name, actionproc_t proc, int64_t data);
void        G_ActionTicker(void);
void        G_ExecuteCommand(char* action);
void        G_BindActionByName(char* key, char* action);
boolean    G_BindActionByEvent(event_t* ev, char* action);
void        G_ShowBinding(char* key);
void        G_GetActionBindings(char* buff, char* action);
void        G_UnbindAction(char* action);
int         G_ListCommands(void);
void        G_OutputBindings(FILE* fh);
void		G_DoCmdGamepadMove(int lx, int ly, int rx, int ry);
void        G_DoCmdMouseMove(float x, float y);

extern boolean    ButtonAction;

#endif
