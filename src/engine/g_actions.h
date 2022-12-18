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

typedef void (*actionproc_t)(int64_t data, int8_t** param);

#define CMD(name) void CMD_ ## name(int64_t data, int8_t** param)

void        G_InitActions(void);
dboolean    G_ActionResponder(event_t* ev);
void        G_AddCommand(int8_t* name, actionproc_t proc, int64_t data);
void        G_ActionTicker(void);
void        G_ExecuteCommand(int8_t* action);
void        G_BindActionByName(int8_t* key, int8_t* action);
dboolean    G_BindActionByEvent(event_t* ev, int8_t* action);
void        G_ShowBinding(int8_t* key);
void        G_GetActionBindings(int8_t* buff, int8_t* action);
void        G_UnbindAction(int8_t* action);
int         G_ListCommands(void);
void        G_OutputBindings(FILE* fh);
void        G_DoCmdMouseMove(int x, int y);

extern dboolean    ButtonAction;

#endif
