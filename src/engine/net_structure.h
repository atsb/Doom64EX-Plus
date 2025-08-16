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
//-------------------------------------------------------------------------------

#ifndef NET_STRUCTRW_H
#define NET_STRUCTRW_H

#include "md5.h"
#include "net_defs.h"

extern void NET_WriteSettings(net_packet_t* packet, net_gamesettings_t* settings);
extern boolean NET_ReadSettings(net_packet_t* packet, net_gamesettings_t* settings);

extern void NET_WriteQueryData(net_packet_t* packet, net_querydata_t* querydata);
extern boolean NET_ReadQueryData(net_packet_t* packet, net_querydata_t* querydata);

extern void NET_WriteTiccmdDiff(net_packet_t* packet, net_ticdiff_t* diff, boolean lowres_turn);
extern boolean NET_ReadTiccmdDiff(net_packet_t* packet, net_ticdiff_t* diff, boolean lowres_turn);
extern void NET_TiccmdDiff(ticcmd_t* tic1, ticcmd_t* tic2, net_ticdiff_t* diff);
extern void NET_TiccmdPatch(ticcmd_t* src, net_ticdiff_t* diff, ticcmd_t* dest);

boolean NET_ReadFullTiccmd(net_packet_t* packet, net_full_ticcmd_t* cmd, boolean lowres_turn);
void NET_WriteFullTiccmd(net_packet_t* packet, net_full_ticcmd_t* cmd, boolean lowres_turn);

boolean NET_ReadMD5Sum(net_packet_t* packet, md5_digest_t digest);
void NET_WriteMD5Sum(net_packet_t* packet, md5_digest_t digest);

#endif /* #ifndef NET_STRUCTRW_H */
