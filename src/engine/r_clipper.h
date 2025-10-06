// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2003 Tim Stump
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

#ifndef R_CLIPPER_H
#define R_CLIPPER_H

#include "tables.h"
#include "gl_main.h"


boolean    R_Clipper_SafeCheckRange(angle_t startAngle, angle_t endAngle);
void        R_Clipper_SafeAddClipRange(angle_t startangle, angle_t endangle);
void        R_Clipper_Clear(void);

extern float frustum[6][4];

angle_t     R_FrustumAngle(void);
void        R_FrustrumSetup(void);
boolean    R_FrustrumTestVertex(vtx_t* vertex, int count);

#endif
