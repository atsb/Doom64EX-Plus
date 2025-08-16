// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2007-2012 Samuel Villarreal
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

#ifndef __F_FINALE__
#define __F_FINALE__

//
// FINALE
//

void    F_Start(void);
void    F_Stop(void);
int     F_Ticker(void);     // Called by main loop.
void    F_Drawer(void);

void    IN_Start(void);
void    IN_Stop(void);
void    IN_Drawer(void);
int     IN_Ticker(void);

#endif
