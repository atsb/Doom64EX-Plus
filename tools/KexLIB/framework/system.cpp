// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2014 Samuel Villarreal
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
//
// DESCRIPTION: Base System API
//
//-----------------------------------------------------------------------------

#include "kexlib.h"

//
// kexSystem::kexSystem
//

kexSystem::kexSystem(void) {
    this->bShuttingDown = false;
}

//
// kexSystem::Sleep
//

void kexSystem::Sleep(unsigned long usecs) {
}

//
// kexSystem::GetMS
//

int kexSystem::GetMS(void) {
    return 0;
}

//
// kexSystem::GetPerformanceCounter
//

uint64_t kexSystem::GetPerformanceCounter(void) {
    return 0;
}

//
// kexSystem::GetTicks
//

int kexSystem::GetTicks(void) {
    return 0;
}

//
// kexSystem::SpawnInternalConsole
//

void kexSystem::SpawnInternalConsole(void) {
}

//
// kexSystem::ShowInternalConsole
//

void kexSystem::ShowInternalConsole(bool show) {
}

//
// kexSystem::DestroyInternalConsole
//

void kexSystem::DestroyInternalConsole(void) {
}

//
// kexSystem::GetWindowFlags
//

int kexSystem::GetWindowFlags(void) {
    return 0;
}

//
// kexSystem::Log
//

void kexSystem::Log(const char *fmt, ...) {
}

//
// kexSystem::GetWindowTitle
//

const char *kexSystem::GetWindowTitle(void) {
    return NULL;
}

//
// kexSystem::SetWindowTitle
//

void kexSystem::SetWindowTitle(const char *string) {
}

//
// kexSystem::SetWindowGrab
//

void kexSystem::SetWindowGrab(const bool bEnable) {
}

//
// kexSystem::WarpMouseToCenter
//

void kexSystem::WarpMouseToCenter(void) {
}

//
// kexSystem::SwapLE16
//

short kexSystem::SwapLE16(const short val) {
    return val;
}

//
// kexSystem::SwapBE16
//

short kexSystem::SwapBE16(const short val) {
    return val;
}

//
// kexSystem::SwapLE32
//

int kexSystem::SwapLE32(const int val) {
    return val;
}

//
// kexSystem::SwapBE32
//

int kexSystem::SwapBE32(const int val) {
    return val;
}

//
// kexSystem::GetProcAddress
//

void *kexSystem::GetProcAddress(const char *proc) {
    return NULL;
}

//
// kexSystem::CheckParam
//

int kexSystem::CheckParam(const char *check) {
    for(int i = 1; i < argc; i++) {
        if(!kexStr::Compare(check, argv[i])) {
            return i;
        }
    }

    return 0;
}

//
// kexSystem::Printf
//

void kexSystem::Printf(const char *string, ...) {
}

//
// kexSystem::CPrintf
//

void kexSystem::CPrintf(rcolor color, const char *string, ...) {
}

//
// kexSystem::Warning
//

void kexSystem::Warning(const char *string, ...) {
}

//
// kexSystem::DPrintf
//

void kexSystem::DPrintf(const char *string, ...) {
}

//
// kexSystem::Error
//

void kexSystem::Error(const char* string, ...) {
}

//
// kexSystem::GetBaseDirectory
//

const char *kexSystem::GetBaseDirectory(void) {
    return NULL;
}
