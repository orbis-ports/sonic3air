/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef SDL_config_orbis_h_
#define SDL_config_orbis_h_

/*
 *  SDL_config_orbis.h - PlayStation 4 (OpenOrbis toolchain, musl headers over Sony's FreeBSD libc).
 *
 *  Selected by include/SDL_config.h when __ORBIS__ is defined (the orbis-compat toolchain file
 *  passes -D__ORBIS__ -D__PS4__). Lives in Oxygen/sonic3air/build/_ps4/sdl/, which the PS4
 *  CMakeLists puts on the include path of the SDL target and of every consumer.
 *
 *  The platform drivers are OPT-IN, switched by CMake options that turn into -D flags on the SDL
 *  target (PUBLIC, so engine TUs see the same configuration):
 *
 *    CMake option             -D passed                  enables here
 *    PS4_SDL_ORBIS_AUDIO      PS4_SDL_ORBIS_AUDIO=1      SDL_AUDIO_DRIVER_ORBIS    (src/audio/orbis/)
 *    PS4_SDL_ORBIS_JOYSTICK   PS4_SDL_ORBIS_JOYSTICK=1   SDL_JOYSTICK_ORBIS        (src/joystick/orbis/)
 *    PS4_SDL_ORBIS_VIDEO      PS4_SDL_ORBIS_VIDEO=1      SDL_VIDEO_DRIVER_ORBIS    (src/video/orbis/)
 *                                                        SDL_VIDEO_OPENGL_EGL, SDL_VIDEO_OPENGL,
 *                                                        SDL_VIDEO_OPENGL_ES2, SDL_VIDEO_RENDER_OGL_ES2
 *
 *  With all three OFF (the default) the build uses SDL's dummy drivers: no picture, no sound, no
 *  pad - but the engine boots, loads its data and runs its scripts, which is milestone M1.
 */

#include "SDL_platform.h"

#define SIZEOF_VOIDP 8
#define HAVE_GCC_ATOMICS 1
#define HAVE_GCC_SYNC_LOCK_TEST_AND_SET 1

#define HAVE_LIBC 1

/* Useful headers */
#define STDC_HEADERS 1
#define HAVE_ALLOCA_H 1
#define HAVE_CTYPE_H 1
#define HAVE_FLOAT_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_LIMITS_H 1
#define HAVE_MATH_H 1
#define HAVE_MEMORY_H 1
#define HAVE_SIGNAL_H 1
#define HAVE_STDARG_H 1
#define HAVE_STDDEF_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_WCHAR_H 1
/* orbis-compat ships pthread_np.h */
#define HAVE_PTHREAD_NP_H 1

/* C library functions */
#define HAVE_MALLOC 1
#define HAVE_CALLOC 1
#define HAVE_REALLOC 1
#define HAVE_FREE 1
#define HAVE_ALLOCA 1
#define HAVE_GETENV 1
#define HAVE_SETENV 1
#define HAVE_PUTENV 1
#define HAVE_UNSETENV 1
#define HAVE_QSORT 1
#define HAVE_BSEARCH 1
#define HAVE_ABS 1
#define HAVE_BCOPY 1
#define HAVE_MEMSET 1
#define HAVE_MEMCPY 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMCMP 1
/* NO HAVE_WCSLEN/WCSDUP/WCSSTR/WCSCMP/WCSNCMP/WCSCASECMP/WCSNCASECMP: SDL is C, and the SDK's C headers
 * make wchar_t 16-bit, while the libc wide functions it would call are 32-bit now (they serve C++ -
 * see _ps4/orbis/orbis_wchar32.c). SDL's own SDL_wcs* loops use its own wchar_t consistently. */
#define HAVE_STRLEN 1
#define HAVE_STRLCPY 1
#define HAVE_STRLCAT 1
#define HAVE_INDEX 1
#define HAVE_RINDEX 1
#define HAVE_STRCHR 1
#define HAVE_STRRCHR 1
#define HAVE_STRSTR 1
#define HAVE_STRTOK_R 1
#define HAVE_STRTOL 1
#define HAVE_STRTOUL 1
#define HAVE_STRTOLL 1
#define HAVE_STRTOULL 1
#define HAVE_STRTOD 1
#define HAVE_ATOI 1
#define HAVE_ATOF 1
#define HAVE_STRCMP 1
#define HAVE_STRNCMP 1
#define HAVE_STRCASECMP 1
#define HAVE_STRNCASECMP 1
#define HAVE_STRCASESTR 1
#define HAVE_SSCANF 1
#define HAVE_VSSCANF 1
#define HAVE_VSNPRINTF 1
#define HAVE_M_PI 1
#define HAVE_ACOS 1
#define HAVE_ACOSF 1
#define HAVE_ASIN 1
#define HAVE_ASINF 1
#define HAVE_ATAN 1
#define HAVE_ATANF 1
#define HAVE_ATAN2 1
#define HAVE_ATAN2F 1
#define HAVE_CEIL 1
#define HAVE_CEILF 1
#define HAVE_COPYSIGN 1
#define HAVE_COPYSIGNF 1
#define HAVE_COS 1
#define HAVE_COSF 1
#define HAVE_EXP 1
#define HAVE_EXPF 1
#define HAVE_FABS 1
#define HAVE_FABSF 1
#define HAVE_FLOOR 1
#define HAVE_FLOORF 1
#define HAVE_FMOD 1
#define HAVE_FMODF 1
#define HAVE_LOG 1
#define HAVE_LOGF 1
#define HAVE_LOG10 1
#define HAVE_LOG10F 1
#define HAVE_LROUND 1
#define HAVE_LROUNDF 1
#define HAVE_POW 1
#define HAVE_POWF 1
#define HAVE_ROUND 1
#define HAVE_ROUNDF 1
#define HAVE_SCALBN 1
#define HAVE_SCALBNF 1
#define HAVE_SIN 1
#define HAVE_SINF 1
#define HAVE_SQRT 1
#define HAVE_SQRTF 1
#define HAVE_TAN 1
#define HAVE_TANF 1
#define HAVE_TRUNC 1
#define HAVE_TRUNCF 1
#define HAVE_FSEEKO 1
#define HAVE_SETJMP 1
#define HAVE_NANOSLEEP 1
#define HAVE_SYSCONF 1
#define HAVE_CLOCK_GETTIME 1
/* Deliberately NOT defined:
 *   HAVE_DLOPEN        - no dynamic loading into a fake-signed eboot
 *   HAVE_ICONV         - SDL's built-in UTF-8/16/32 converter is used instead
 *   HAVE_SIGACTION     - the SDK's SA_* values are Linux's, the kernel reads FreeBSD's (see
 *                        orbis-compat/include/signal.h); SDL does not need signals here
 *   HAVE_SYSCTLBYNAME  - SDL_cpuinfo falls back to sysconf
 *   HAVE_PTHREAD_SETNAME_NP / HAVE_PTHREAD_SET_NAME_NP - thread names are cosmetic
 *   HAVE_SEM_TIMEDWAIT - POSIX semaphores do not work on the console; generic/SDL_syssem.c is built
 *   HAVE_POLL, HAVE_MEMFD_CREATE, HAVE_POSIX_FALLOCATE, HAVE_GETAUXVAL, HAVE_ELF_AUX_INFO
 */

/* Subsystems with no platform implementation at all */
#define SDL_HAPTIC_DISABLED 1
#define SDL_HIDAPI_DISABLED 1
#define SDL_SENSOR_DISABLED 1
#define SDL_LOADSO_DISABLED 1
#define SDL_POWER_DISABLED 1

/* Stub drivers */
#define SDL_HAPTIC_DUMMY 1
#define SDL_SENSOR_DUMMY 1
#define SDL_LOADSO_DUMMY 1
#define SDL_FILESYSTEM_DUMMY 1
#define SDL_MISC_DUMMY 1
#define SDL_LOCALE_DUMMY 1

/* Threads and timers: the SDK's pthreads and clock_gettime work (orbis-compat corrects the types) */
#define SDL_THREAD_PTHREAD 1
#define SDL_THREAD_PTHREAD_RECURSIVE_MUTEX 1
#define SDL_TIMER_UNIX 1

/* ------------------------------------------------------------------ audio */
#define SDL_AUDIO_DRIVER_DUMMY 1
#define SDL_AUDIO_DRIVER_DISK 1
#if defined(PS4_SDL_ORBIS_AUDIO) && PS4_SDL_ORBIS_AUDIO
#define SDL_AUDIO_DRIVER_ORBIS 1
#endif

/* ------------------------------------------------------------------ joystick */
#if defined(PS4_SDL_ORBIS_JOYSTICK) && PS4_SDL_ORBIS_JOYSTICK
#define SDL_JOYSTICK_ORBIS 1
#else
#define SDL_JOYSTICK_DUMMY 1
#endif

/* ------------------------------------------------------------------ video */
#define SDL_VIDEO_DRIVER_DUMMY 1
#if defined(PS4_SDL_ORBIS_VIDEO) && PS4_SDL_ORBIS_VIDEO
#define SDL_VIDEO_DRIVER_ORBIS 1
#define SDL_VIDEO_OPENGL_EGL 1
/* Desktop GL (4.6 core through zink) is what the engine asks for; ES2 is the SDL_Renderer fallback */
#define SDL_VIDEO_OPENGL 1
#define SDL_VIDEO_OPENGL_ES2 1
#define SDL_VIDEO_RENDER_OGL_ES2 1
#endif

#endif /* SDL_config_orbis_h_ */
