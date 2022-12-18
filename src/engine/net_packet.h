// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2005 Simon Howard
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
// DESCRIPTION:
//     Definitions for use in networking code.
//
//-----------------------------------------------------------------------------

#ifndef NET_PACKET_H
#define NET_PACKET_H

#include "net_defs.h"

net_packet_t* NET_NewPacket(int initial_size);
net_packet_t* NET_PacketDup(net_packet_t* packet);
void NET_FreePacket(net_packet_t* packet);

dboolean NET_ReadInt8(net_packet_t* packet, int* data);
dboolean NET_ReadInt16(net_packet_t* packet, uint32_t* data);
dboolean NET_ReadInt32(net_packet_t* packet, uint32_t* data);

dboolean NET_ReadSInt8(net_packet_t* packet, int* data);
dboolean NET_ReadSInt16(net_packet_t* packet, int32_t* data);
dboolean NET_ReadSInt32(net_packet_t* packet, int32_t* data);

char* NET_ReadString(net_packet_t* packet);

void NET_WriteInt8(net_packet_t* packet, uint32_t i);
void NET_WriteInt16(net_packet_t* packet, uint32_t i);
void NET_WriteInt32(net_packet_t* packet, uint32_t i);

void NET_WriteString(net_packet_t* packet, char* string);

#endif /* #ifndef NET_PACKET_H */
