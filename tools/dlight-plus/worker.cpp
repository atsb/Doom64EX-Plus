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
//-----------------------------------------------------------------------------
//
// DESCRIPTION: Worker Threads
//
//-----------------------------------------------------------------------------

#include "common.h"
#include "worker.h"

int kexWorker::maxWorkThreads = 4;

//
// WorkThread
//

void *WorkThread(void *p)
{
    jobFuncArgs_t *args = (jobFuncArgs_t*)p;
    kexWorker *worker = args->worker;

    while(1)
    {
        int jobid;

        worker->LockMutex();

        if(worker->FinishedAllJobs())
        {
            worker->UnlockMutex();
            break;
        }

        jobid = worker->DispatchJob();
        worker->UnlockMutex();

        worker->RunJob(args->data, jobid);
    }

    pthread_exit(NULL);
    return 0;
}

//
// kexWorker::kexWorker
//

kexWorker::kexWorker(void)
{
    this->numWorkLoad = 0;
    this->jobsWorked = 0;
    this->job = NULL;

#ifdef KEX_WIN32
    this->mutex = NULL;
#endif

    memset(this->jobArgs, 0, sizeof(this->jobArgs));
}

//
// kexWorker::~kexWorker
//

kexWorker::~kexWorker(void)
{
}

//
// kexWorker::LockMutex
//

void kexWorker::LockMutex(void)
{
#ifdef KEX_WIN32
    if(mutex)
    {
#endif
        pthread_mutex_lock(&mutex);
#ifdef KEX_WIN32
    }
#endif
}

//
// kexWorker::UnlockMutex
//

void kexWorker::UnlockMutex(void)
{
#ifdef KEX_WIN32
    if(mutex)
    {
#endif
        pthread_mutex_unlock(&mutex);
#ifdef KEX_WIN32
    }
#endif
}

//
// kexWorker::Destroy
//

void kexWorker::Destroy(void)
{
#ifdef KEX_WIN32
    if(this->mutex)
    {
#endif
        pthread_mutex_destroy(&this->mutex);
#ifdef KEX_WIN32
    }
#endif
}

//
// kexWorker::RunThreads
//

void kexWorker::RunThreads(const int count, void *data, jobFunc_t jobFunc)
{
    pthread_attr_t attr;
    void *status;
    int rc;

    job = jobFunc;
    numWorkLoad = count;
    jobsWorked = 0;

#ifdef KEX_WIN32
    if(!mutex)
    {
#endif
        if((rc = pthread_mutex_init(&mutex, NULL)))
        {
            Error("pthread_mutex_init failed (error code %i)\n", rc);
            return;
        }
#ifdef KEX_WIN32
    }
#endif

    if((rc = pthread_attr_init(&attr)))
    {
        Error("pthread_attr_init failed (error code %i)\n", rc);
        return;
    }

    if((rc = pthread_attr_setstacksize(&attr, 1024 * 1024)))
    {
        Error("pthread_attr_setstacksize failed (error code %i)\n", rc);
        return;
    }

    if((rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE)))
    {
        Error("pthread_attr_setdetachstate failed (error code %i)\n", rc);
        return;
    }

    for(int i = 0; i < kexWorker::maxWorkThreads; ++i)
    {
        jobArgs[i].worker = this;
        jobArgs[i].jobID = i;
        jobArgs[i].data = data;

        if((rc = pthread_create(&threads[i], &attr, WorkThread, (void*)&jobArgs[i])))
        {
            Error("pthread_create failed (error code %i)\n", rc);
            return;
        }
    }

    pthread_attr_destroy(&attr);

    for(int i = 0; i < kexWorker::maxWorkThreads; ++i)
    {
        if((rc = pthread_join(threads[i], &status)))
        {
            Error("pthread_join failed (error code %i)\n", rc);
            return;
        }
    }
}
