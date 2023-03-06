#ifndef CONFIG_H
#define CONFIG_H

/* Define to enable ALSA driver */
// #define ALSA_SUPPORT 0

/* Define to activate sound output to files */
// #define AUFILE_SUPPORT 0

/* whether or not we are supporting CoreAudio */
// #define COREAUDIO_SUPPORT 0

/* whether or not we are supporting CoreMIDI */
// #define COREMIDI_SUPPORT 0

/* whether or not we are supporting DART */
// #define DART_SUPPORT 0

/* Define if building for Mac OS X Darwin */
// #define DARWIN 0

/* Define if D-Bus support is enabled */
// #define DBUS_SUPPORT 0

/* Soundfont to load automatically in some use cases */
#define DEFAULT_SOUNDFONT "ux0:/data/default.sf2"

/* Define to enable FPE checks */
// #define FPE_CHECK 0

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
// #define HAVE_DLFCN_H 0

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* whether or not we are supporting ladcca */
// #define HAVE_LADCCA 0

/* whether or not we are supporting lash */
// #define HAVE_LASH 0

/* Define to 1 if you have the `dl' library (-ldl). */
// #define HAVE_LIBDL 0

/* Define to 1 if you have the `MidiShare' library (-lMidiShare). */
// #define HAVE_LIBMIDISHARE 0

/* Define to 1 if you have the `pthread' library (-lpthread). */
#define HAVE_LIBPTHREAD 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the <math.h> header file. */
#define HAVE_MATH_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* Define to 1 if you have the <netinet/tcp.h> header file. */
#define HAVE_NETINET_TCP_H 1

/* Define to 1 if you have the <pthread.h> header file. */
#define HAVE_PTHREAD_H 1

/* Define to 1 if you have the <stdarg.h> header file. */
#define HAVE_STDARG_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdbool.h> header file. */
#define HAVE_STDBOOL_H 1

/* Define to 1 if you have the <stdatomic.h> header file. */
#define HAVE_STDATOMIC_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <threads.h> header file. */
#define HAVE_THREADS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <getopt.h> header file. */
#define HAVE_GETOPT_H 1

/* Name of package */
#define PACKAGE "fluidsynth"

/* Define to the full name of this package. */
#define PACKAGE_NAME "fluidsynth-lite"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "fluidsynth-lite-1.1.6-vita"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME @PACKAGE_TARNAME@

/* Define to the version of this package. */
#define PACKAGE_VERSION @PACKAGE_VERSION@

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "1.1.6"

/* Define to do all DSP in single floating point precision */
#define WITH_FLOAT 1

/* Define if the compiler supports VLA */ 
#define SUPPORTS_VLA 1

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef inline
#define inline __inline
#endif

#endif /* CONFIG_H */
