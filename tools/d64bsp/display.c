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

#include "glbsp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <assert.h>

#if defined(MSDOS) || defined(__MSDOS__)
#include <dos.h>
#endif

#if defined(UNIX) || defined(__UNIX__)
#include <unistd.h>  // Unix: isatty()
#include <signal.h>  // Unix: raise()
#endif

#include "display.h"


#define FATAL_COREDUMP  0


static boolean_g disable_progress = FALSE;

static displaytype_e curr_disp = DIS_INVALID;

static int progress_target = 0;
static int progress_shown;


const nodebuildfuncs_t cmdline_funcs =
{
  TextFatalError,
  TextPrintMsg,
  TextTicker,

  TextDisplayOpen,
  TextDisplaySetTitle,
  TextDisplaySetBar,
  TextDisplaySetBarLimit,
  TextDisplaySetBarText,
  TextDisplayClose
};


//
// TextStartup
//
void TextStartup(void)
{
  setbuf(stdout, NULL);

#ifdef UNIX  
  // no whirling baton if stderr is redirected
  if (! isatty(2))
    disable_progress = TRUE;
#endif
}

//
// TextShutdown
//
void TextShutdown(void)
{
  /* nothing to do */
}

//
// TextDisableProgress
//
void TextDisableProgress(void)
{
  disable_progress = TRUE;
}

//
// TextPrintMsg
//
void TextPrintMsg(const char *str, ...)
{
  va_list args;

  va_start(args, str);
  vprintf(str, args);
  va_end(args);

  fflush(stdout);
}

//
// TextFatalError
//
// Terminates the program reporting an error.
//
void TextFatalError(const char *str, ...)
{
  va_list args;

  va_start(args, str);
  vfprintf(stderr, str, args);
  va_end(args);

#if FATAL_COREDUMP && defined(UNIX)
  raise(SIGSEGV);
#endif

  exit(5);
}

//
// TextTicker
//
void TextTicker(void)
{
  /* does nothing */
}


static void ClearProgress(void)
{
  fprintf(stderr, "                \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
}

//
// TextDisplayOpen
//
boolean_g TextDisplayOpen(displaytype_e type)
{
  // shutdown any existing display
  TextDisplayClose();

  switch (type)
  {
    case DIS_BUILDPROGRESS:
    case DIS_FILEPROGRESS:
      // these are OK
      break;

    default:
      curr_disp = DIS_INVALID;
      return FALSE;
  }

  curr_disp = type;
  progress_target = 0;

  return TRUE;
}

//
// TextDisplaySetTitle
//
void TextDisplaySetTitle(const char *str)
{
  /* does nothing */
}

//
// TextDisplaySetBarText
//
void TextDisplaySetBarText(int barnum, const char *str)
{
  /* does nothing */
}

//
// TextDisplaySetBarLimit
//
void TextDisplaySetBarLimit(int barnum, int limit)
{
  if (curr_disp == DIS_INVALID || disable_progress)
    return;
 
  // select the correct bar
  if ((curr_disp == DIS_FILEPROGRESS && barnum != 1) ||
      (curr_disp == DIS_BUILDPROGRESS && barnum != 1))
  {
    return;
  }
 
  progress_target = limit;
  progress_shown  = -1;
}

//
// TextDisplaySetBar
//
void TextDisplaySetBar(int barnum, int count)
{
  int perc;
  
  if (curr_disp == DIS_INVALID || disable_progress ||
      progress_target <= 0)
  {
    return;
  }

  // select the correct bar
  if ((curr_disp == DIS_FILEPROGRESS && barnum != 1) ||
      (curr_disp == DIS_BUILDPROGRESS && barnum != 1))
  {
    return;
  }
 
  if (count > progress_target)
    TextFatalError("\nINTERNAL ERROR: Progress went past target !\n\n");
 
  perc = count * 100 / progress_target;

  if (perc == progress_shown)
    return;

  if (perc == 0)
    ClearProgress();
  else
    fprintf(stderr, "--%3d%%--\b\b\b\b\b\b\b\b", perc);

  progress_shown = perc;
}

//
// TextDisplayClose
//
void TextDisplayClose(void)
{
  if (curr_disp == DIS_INVALID || disable_progress)
    return;

  ClearProgress();
}

