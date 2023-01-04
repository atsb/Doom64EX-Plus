//------------------------------------------------------------------------
// DISPLAY : Command-line display routines
//------------------------------------------------------------------------
//
//  GL-Friendly Node Builder (C) 2000-2007 Andrew Apted
//
//  Based on 'BSP 2.3' by Colin Reed, Lee Killough and others.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

#ifndef __CMDLINE_DISPLAY_H__
#define __CMDLINE_DISPLAY_H__


extern const nodebuildfuncs_t cmdline_funcs;

void TextStartup(void);
void TextShutdown(void);
void TextDisableProgress(void);

void TextFatalError(const char *str, ...) GCCATTR((format (printf, 1, 2)));
void TextPrintMsg(const char *str, ...) GCCATTR((format (printf, 1, 2)));
void TextTicker(void);

boolean_g TextDisplayOpen(displaytype_e type);
void TextDisplaySetTitle(const char *str);
void TextDisplaySetBar(int barnum, int count);
void TextDisplaySetBarLimit(int barnum, int limit);
void TextDisplaySetBarText(int barnum, const char *str);
void TextDisplayClose(void);


#endif /* __CMDLINE_DISPLAY_H__ */
