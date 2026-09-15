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
#include "../../SDL_internal.h"

#ifdef SDL_AUDIO_DRIVER_ORBIS

/* PS4 (Orbis) audio output through sceAudioOut.
 *
 * Hardware facts this driver is built around (measured by the RetroArch and
 * OpenGothic PS4 ports, see audio/drivers/ps4_audio.c and ps4/og_sound_orbis.cpp):
 *
 *  - sceAudioOutOpen wants the SYSTEM user (0xFF), not the logged-in one
 *    (a real user id fails with 0x809b0001). This is the opposite of scePadOpen.
 *  - The port opens silent on some firmwares: sceAudioOutSetVolume is mandatory.
 *  - sceAudioOutClose returns success but the port is never given back; after
 *    eight open/close cycles sceAudioOutOpen fails with PORT_FULL. So the port is
 *    opened once per process, cached in a static, and reused on every reopen.
 *  - The MAIN port takes 48 kHz only, and every sceAudioOutOutput must carry
 *    exactly one grain. The call blocks until the grain has been consumed, so it
 *    is the clock of SDL's audio thread: WaitDevice is a no-op.
 *
 * The device spec is forced to 48000 Hz / S16LSB / stereo, and spec.samples is
 * rounded up to a multiple of the 256-frame grain (sonic3air asks for 1024, which
 * then needs no SDL_AudioStream at all). PlayDevice chunks the buffer into grains.
 */

#include "SDL_audio.h"
#include "SDL_error.h"
#include "SDL_log.h"
#include "../SDL_audio_c.h"
#include "../SDL_sysaudio.h"
#include "SDL_orbisaudio.h"

#include <stdarg.h>
#include <stdint.h>

#include <orbis/AudioOut.h>
#include <orbis/UserService.h>

/* orbis-compat: routed to the UDP netlog once ps4_app_init() has run. */
#include <orbis_log.h>

/* libkernel; microseconds. Declared here rather than through <orbis/libkernel.h>,
   which drags in far more than this file needs. */
extern uint64_t sceKernelGetProcessTime(void);

#define ORBIS_AUDIO_MAX_SAMPLES (ORBIS_AUDIO_GRAIN * 32) /* 8192 frames, 170 ms */

/* A gap this long between the end of one grain and the start of the next means the
   port ran dry: the mixer callback (or the scheduler) did not keep up. Two grains. */
#define ORBIS_AUDIO_LATE_US ((Uint64)ORBIS_AUDIO_GRAIN * 2 * 1000000 / ORBIS_AUDIO_RATE)

/* Statistics are reported at most this often, and only when something moved. */
#define ORBIS_AUDIO_REPORT_US (10 * 1000000)

static void ORBISAUD_Log(const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(1);
static void ORBISAUD_Log(const char *fmt, ...)
{
    char line[256];
    va_list ap;

    va_start(ap, fmt);
    SDL_vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);

    if (orbis_log_enabled()) {
        orbis_log("S3AIR_SDL_AUDIO: %s", line);
    } else {
        SDL_Log("S3AIR_SDL_AUDIO: %s", line);
    }
}

/* Process-wide port state. See the header comment for why it is never closed. */
static SDL_bool orbis_audio_service_up = SDL_FALSE;
static int32_t orbis_audio_port = -1;
static SDL_bool orbis_audio_port_busy = SDL_FALSE; /* an SDL device currently owns it */

/* Diagnostics, audio thread only. */
static Uint64 orbis_audio_grains = 0;
static Uint64 orbis_audio_errors = 0;
static Uint64 orbis_audio_late = 0;
static Uint64 orbis_audio_errors_said = 0;
static Uint64 orbis_audio_late_said = 0;
static Uint64 orbis_audio_last_end_us = 0;
static Uint64 orbis_audio_last_report_us = 0;
static int32_t orbis_audio_last_error = 0;

static SDL_bool ORBISAUD_BringUpService(void)
{
    OrbisUserServiceInitializeParams params;
    int32_t user_rc, init_rc;

    if (orbis_audio_service_up) {
        return SDL_TRUE;
    }

    SDL_zero(params);
    params.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
    user_rc = sceUserServiceInitialize(&params);

    init_rc = sceAudioOutInit();

    /* ALREADY_INIT (0x8026000E) is what a second init in the same process returns;
       0x8026000D is OUT_OF_MEMORY and is a real failure. */
    ORBISAUD_Log("init: sceUserServiceInitialize rc=0x%08x%s, sceAudioOutInit rc=0x%08x%s",
                 (unsigned)user_rc,
                 (user_rc == (int32_t)ORBIS_USER_SERVICE_ERROR_ALREADY_INITIALIZED) ? " (already initialized)" : "",
                 (unsigned)init_rc,
                 (init_rc == (int32_t)ORBIS_AUDIO_OUT_ERROR_ALREADY_INIT) ? " (already initialized)" : "");

    if (init_rc != 0 && init_rc != (int32_t)ORBIS_AUDIO_OUT_ERROR_ALREADY_INIT) {
        SDL_SetError("sceAudioOutInit failed: 0x%08x", (unsigned)init_rc);
        return SDL_FALSE;
    }

    orbis_audio_service_up = SDL_TRUE;
    return SDL_TRUE;
}

static int ORBISAUD_OpenDevice(_THIS, const char *devname)
{
    int32_t vol[8];
    int32_t vol_rc;
    int samples;
    int i;

    const int asked_freq = _this->spec.freq;
    const SDL_AudioFormat asked_format = _this->spec.format;
    const int asked_channels = _this->spec.channels;
    const int asked_samples = _this->spec.samples;

    if (_this->iscapture) {
        return SDL_SetError("orbis audio: capture is not supported");
    }

    if (!ORBISAUD_BringUpService()) {
        return -1;
    }

    if (orbis_audio_port_busy) {
        return SDL_SetError("orbis audio: the MAIN port is already owned by another open device");
    }

    /* The port has one format. SDL converts whatever the app asked for into this. */
    _this->spec.freq = ORBIS_AUDIO_RATE;
    _this->spec.format = AUDIO_S16LSB;
    _this->spec.channels = ORBIS_AUDIO_CHANNELS;

    /* A whole number of grains, at least one. */
    samples = _this->spec.samples;
    if (samples < ORBIS_AUDIO_GRAIN) {
        samples = ORBIS_AUDIO_GRAIN;
    }
    if (samples > ORBIS_AUDIO_MAX_SAMPLES) {
        samples = ORBIS_AUDIO_MAX_SAMPLES;
    }
    samples = ((samples + ORBIS_AUDIO_GRAIN - 1) / ORBIS_AUDIO_GRAIN) * ORBIS_AUDIO_GRAIN;
    _this->spec.samples = (Uint16)samples;

    SDL_CalculateAudioSpec(&_this->spec);

    _this->hidden = (struct SDL_PrivateAudioData *)SDL_calloc(1, sizeof(*_this->hidden));
    if (!_this->hidden) {
        return SDL_OutOfMemory();
    }

    _this->hidden->grains = samples / ORBIS_AUDIO_GRAIN;
    _this->hidden->mixbuf = (Uint8 *)SDL_malloc(_this->spec.size);
    if (!_this->hidden->mixbuf) {
        return SDL_OutOfMemory();
    }
    SDL_memset(_this->hidden->mixbuf, _this->spec.silence, _this->spec.size);

    if (orbis_audio_port < 0) {
        const int32_t port = sceAudioOutOpen(ORBIS_USER_SERVICE_USER_ID_SYSTEM,
                                             ORBIS_AUDIO_OUT_PORT_TYPE_MAIN,
                                             0,
                                             ORBIS_AUDIO_GRAIN,
                                             ORBIS_AUDIO_RATE,
                                             ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_STEREO);
        ORBISAUD_Log("open: sceAudioOutOpen(user=0xFF, MAIN, 0, grain=%d, %d Hz, S16_STEREO) rc=0x%08x%s",
                     ORBIS_AUDIO_GRAIN, ORBIS_AUDIO_RATE, (unsigned)port,
                     ((uint32_t)port == ORBIS_AUDIO_OUT_ERROR_PORT_FULL) ? " (PORT_FULL)" : "");
        if (port < 0) {
            return SDL_SetError("sceAudioOutOpen failed: 0x%08x", (unsigned)port);
        }
        orbis_audio_port = port;
    } else {
        ORBISAUD_Log("open: reusing the process-wide port handle 0x%08x", (unsigned)orbis_audio_port);
    }

    /* Mandatory: the port can open at volume 0 on every channel. Bit i of the
       flag selects channel i; 0xFF covers all of them. */
    for (i = 0; i < 8; i++) {
        vol[i] = 32768;
    }
    vol_rc = sceAudioOutSetVolume(orbis_audio_port, 0xFF, vol);
    ORBISAUD_Log("open: sceAudioOutSetVolume(0x%08x, 0xFF, 32768) rc=0x%08x; device spec %d Hz S16 stereo, "
                 "%d frames (%d grain(s)); app asked %d Hz fmt 0x%04x ch %d samples %d",
                 (unsigned)orbis_audio_port, (unsigned)vol_rc, _this->spec.freq, samples,
                 _this->hidden->grains, asked_freq, (unsigned)asked_format, asked_channels, asked_samples);

    orbis_audio_port_busy = SDL_TRUE;
    orbis_audio_last_end_us = 0;
    orbis_audio_last_report_us = sceKernelGetProcessTime();
    return 0;
}

static void ORBISAUD_Report(Uint64 now_us, SDL_bool force)
{
    if (!force && (now_us - orbis_audio_last_report_us) < ORBIS_AUDIO_REPORT_US) {
        return;
    }
    orbis_audio_last_report_us = now_us;

    if (orbis_audio_errors != orbis_audio_errors_said || orbis_audio_late != orbis_audio_late_said) {
        ORBISAUD_Log("stats: %llu grains, %llu output errors (+%llu, last rc=0x%08x), %llu late grains (+%llu, port ran dry)",
                     (unsigned long long)orbis_audio_grains,
                     (unsigned long long)orbis_audio_errors,
                     (unsigned long long)(orbis_audio_errors - orbis_audio_errors_said),
                     (unsigned)orbis_audio_last_error,
                     (unsigned long long)orbis_audio_late,
                     (unsigned long long)(orbis_audio_late - orbis_audio_late_said));
        orbis_audio_errors_said = orbis_audio_errors;
        orbis_audio_late_said = orbis_audio_late;
    }
}

static void ORBISAUD_PlayDevice(_THIS)
{
    const int bytes_per_grain = ORBIS_AUDIO_GRAIN * ORBIS_AUDIO_CHANNELS * sizeof(Sint16);
    Uint8 *buf = _this->hidden->mixbuf;
    int i;

    for (i = 0; i < _this->hidden->grains; i++) {
        const Uint64 start_us = sceKernelGetProcessTime();
        const int32_t rc = sceAudioOutOutput(orbis_audio_port, buf + (i * bytes_per_grain));
        const Uint64 end_us = sceKernelGetProcessTime();

        if (orbis_audio_last_end_us != 0 && (start_us - orbis_audio_last_end_us) > ORBIS_AUDIO_LATE_US) {
            orbis_audio_late++;
        }
        orbis_audio_last_end_us = end_us;

        if (rc < 0) {
            if (orbis_audio_errors == 0) {
                ORBISAUD_Log("play: first sceAudioOutOutput failure rc=0x%08x", (unsigned)rc);
            }
            orbis_audio_errors++;
            orbis_audio_last_error = rc;
        } else if (orbis_audio_grains == 0) {
            ORBISAUD_Log("play: first grain out, sceAudioOutOutput rc=%d, blocked %llu us",
                         (int)rc, (unsigned long long)(end_us - start_us));
        }
        orbis_audio_grains++;
    }

    ORBISAUD_Report(orbis_audio_last_end_us, SDL_FALSE);
}

static void ORBISAUD_WaitDevice(_THIS)
{
    /* sceAudioOutOutput blocks until the grain is consumed; nothing to wait for. */
}

static Uint8 *ORBISAUD_GetDeviceBuf(_THIS)
{
    return _this->hidden->mixbuf;
}

static void ORBISAUD_CloseDevice(_THIS)
{
    /* The port is deliberately NOT closed: see the header comment. */
    if (orbis_audio_port_busy) {
        ORBISAUD_Report(sceKernelGetProcessTime(), SDL_TRUE);
        ORBISAUD_Log("close: device released, port 0x%08x kept open for reuse", (unsigned)orbis_audio_port);
    }
    orbis_audio_port_busy = SDL_FALSE;

    if (_this->hidden) {
        SDL_free(_this->hidden->mixbuf);
        SDL_free(_this->hidden);
        _this->hidden = NULL;
    }
}

static SDL_bool ORBISAUD_Init(SDL_AudioDriverImpl *impl)
{
    impl->OpenDevice = ORBISAUD_OpenDevice;
    impl->PlayDevice = ORBISAUD_PlayDevice;
    impl->WaitDevice = ORBISAUD_WaitDevice;
    impl->GetDeviceBuf = ORBISAUD_GetDeviceBuf;
    impl->CloseDevice = ORBISAUD_CloseDevice;

    impl->HasCaptureSupport = SDL_FALSE;
    impl->OnlyHasDefaultOutputDevice = SDL_TRUE;
    impl->SupportsNonPow2Samples = SDL_TRUE;

    return SDL_TRUE; /* this audio target is available. */
}

AudioBootStrap ORBISAUD_bootstrap = {
    "orbis", "PS4 sceAudioOut driver", ORBISAUD_Init, SDL_FALSE
};

#endif /* SDL_AUDIO_DRIVER_ORBIS */

/* vi: set ts=4 sw=4 expandtab: */
