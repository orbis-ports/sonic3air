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

#ifndef SDL_orbisaudio_h_
#define SDL_orbisaudio_h_

#include "../SDL_sysaudio.h"

/* Hidden "this" pointer for the audio functions (SDL_sysaudio.h undefines its own) */
#define _THIS SDL_AudioDevice *_this

/* PS4 (Orbis) sceAudioOut: the MAIN port only accepts 48 kHz, and every
   sceAudioOutOutput call must carry exactly one grain of frames. */
#define ORBIS_AUDIO_RATE     48000
#define ORBIS_AUDIO_GRAIN    256
#define ORBIS_AUDIO_CHANNELS 2

struct SDL_PrivateAudioData
{
    Uint8 *mixbuf;      /* spec.size bytes: a whole number of grains */
    int grains;         /* spec.samples / ORBIS_AUDIO_GRAIN */
};

#endif /* SDL_orbisaudio_h_ */

/* vi: set ts=4 sw=4 expandtab: */
