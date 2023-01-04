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

#ifndef __WORKER_H__
#define __WORKER_H__

#include <pthread.h>

#define MAX_THREADS     128

class kexWorker;

typedef void(*jobFunc_t)(void*, int);

typedef struct
{
    kexWorker *worker;
    void *data;
    int jobID;
} jobFuncArgs_t;

class kexWorker
{
public:
    kexWorker(void);
    ~kexWorker(void);

    void                RunThreads(const int count, void *data, jobFunc_t jobFunc);
    void                LockMutex(void);
    void                UnlockMutex(void);
    void                Destroy(void);

    bool                FinishedAllJobs(void) { return jobsWorked == numWorkLoad; }
    int                 DispatchJob(void) { int j = jobsWorked; jobsWorked++; return j; }
    void                RunJob(void *data, const int jobID) { job(data, jobID); }

    jobFuncArgs_t       *Args(const int id) { return &jobArgs[id]; }
    const int           JobsWorked(void) const { return jobsWorked; }

    static int          maxWorkThreads;

private:
    pthread_t           threads[MAX_THREADS];
    jobFuncArgs_t       jobArgs[MAX_THREADS];
    pthread_mutex_t     mutex;
    jobFunc_t           job;
    int                 jobsWorked;
    int                 numWorkLoad;
};

#endif
