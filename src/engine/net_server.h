// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
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
// Network server code
//
//--------------------------------------------------------------------------------

#ifndef NET_SERVER_H
#define NET_SERVER_H

#include "con_cvar.h"
#include "net_defs.h"

// initialise server and wait for connections

void NET_SV_Init(void);

// run server: check for new packets received etc.

void NET_SV_Run(void);

// Shut down the server
// Blocks until all clients disconnect, or until a 5 second timeout

void NET_SV_Shutdown(void);

// Add a network module to the context used by the server

void NET_SV_AddModule(net_module_t* module);

// Update server cvars across all clients if changed by host/listen server

void NET_SV_UpdateCvars(cvar_t* cvar);

#endif /* #ifndef NET_SERVER_H */
