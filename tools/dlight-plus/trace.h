//
// Copyright (c) 2013-2014 Samuel Villarreal
// svkaiser@gmail.com
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//    1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
//   2. Altered source versions must be plainly marked as such, and must not be
//   misrepresented as being the original software.
//
//    3. This notice may not be removed or altered from any source
//    distribution.
//

#ifndef __TRACE_H__
#define __TRACE_H__

class kexDoomMap;

class kexTrace
{
public:
    kexTrace(void);
    ~kexTrace(void);

    void                Init(kexDoomMap &doomMap);
    void                Trace(const kexVec3 &startVec, const kexVec3 &endVec);

    kexVec3             start;
    kexVec3             end;
    kexVec3             dir;
    kexVec3             hitNormal;
    kexVec3             hitVector;
    surface_t           *hitSurface;
    float               fraction;

private:
    void                TraceBSPNode(int num);
    void                TraceSubSector(int num);
    void                TraceSurface(surface_t *surface);

    kexDoomMap          *map;

};

#endif
