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
// DESCRIPTION:
//     Querying servers to find their current status.
//

#ifndef NET_QUERY_H
#define NET_QUERY_H

#include "net_defs.h"

extern void NET_QueryAddress(char* addr);
extern void NET_LANQuery(void);
extern net_addr_t* NET_FindLANServer(void);

#endif /* #ifndef NET_QUERY_H */
