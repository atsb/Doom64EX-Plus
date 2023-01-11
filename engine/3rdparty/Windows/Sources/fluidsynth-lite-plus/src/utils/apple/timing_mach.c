#define _POSIX_C_SOURCE 200809L
#include <unistd.h>

#include "timing_mach.h"

#if TIMING_MACH_BEFORE_10_12
/* ******** */
/* __MACH__ */

#include <mach/mach_time.h>
#include <mach/mach.h>
#include <mach/clock.h>

/* __MACH__ */
/* ******** */
#endif

extern double timespec2secd(const struct timespec *ts_in);
extern void secd2timespec(struct timespec *ts_out, const double sec_d);
extern void timespec_monodiff_lmr(struct timespec *ts_out,
                                  const struct timespec *ts_in);
extern void timespec_monodiff_rml(struct timespec *ts_out,
                                  const struct timespec *ts_in);
extern void timespec_monoadd(struct timespec *ts_out,
                             const struct timespec *ts_in);

#ifdef __MACH__
/* ******** */
/* __MACH__ */

    /* clock_nanosleep for CLOCK_MONOTONIC and TIMER_ABSTIME */
    extern int clock_nanosleep_abstime (const struct timespec *req);

/* __MACH__ */
/* ******** */
#endif


#if TIMING_MACH_BEFORE_10_12
/* ******** */
/* __MACH__ */

    /* timing struct for osx */
    typedef struct RoTimingMach {
        mach_timebase_info_data_t timebase;
        clock_serv_t cclock;
    } RoTimingMach;

    /* internal timing struct for osx */
    static RoTimingMach ro_timing_mach_g;

    /* mach clock port */
    extern mach_port_t clock_port;

    /* emulate posix clock_gettime */
    int clock_gettime (clockid_t id, struct timespec *tspec)
    {
        int retval = -1;
        mach_timespec_t mts;
        if (id == CLOCK_REALTIME) {
            retval = clock_get_time (ro_timing_mach_g.cclock, &mts);
            if (retval == 0) {
                tspec->tv_sec = mts.tv_sec;
                tspec->tv_nsec = mts.tv_nsec;
            }
        } else if (id == CLOCK_MONOTONIC) {
            retval = clock_get_time (clock_port, &mts);
            if (retval == 0) {
                tspec->tv_sec = mts.tv_sec;
                tspec->tv_nsec = mts.tv_nsec;
            }
        } else {}
        return retval;
    }

    /* emulate posix clock_getres */
    int clock_getres (clockid_t id, struct timespec *res)
    {

        (void)id;
        res->tv_sec = 0;
        res->tv_nsec = ro_timing_mach_g.timebase.numer / ro_timing_mach_g.timebase.denom;
        return 0;

    }

    /* initialize */
    int timing_mach_init (void) {
        static int call_count = 0;
        call_count++;
        int retval = -2;
        if (call_count == 1) {
            retval = mach_timebase_info (&ro_timing_mach_g.timebase);
            if (retval == 0) {
                retval = host_get_clock_service (mach_host_self (),
                                                 CALENDAR_CLOCK, &ro_timing_mach_g.cclock);
            }
        } else {
            /* don't overflow, reset call count */
            call_count = 1;
        }
        return retval;
    }

/* __MACH__ */
/* ******** */
#endif

int itimer_start (struct timespec *ts_target, const struct timespec *ts_step) {
    int retval = clock_gettime(CLOCK_MONOTONIC, ts_target);
    if (retval == 0) {
        /* add step size to current monotonic time */
        timespec_monoadd(ts_target, ts_step);
    }
    return retval;
}

int itimer_step (struct timespec *ts_target, const struct timespec *ts_step) {
    int retval = clock_nanosleep_abstime(ts_target);
    if (retval == 0) {
        /* move target along */
        timespec_monoadd(ts_target, ts_step);
    }
    return retval;
}
