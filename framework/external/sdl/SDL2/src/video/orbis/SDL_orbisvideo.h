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

#ifndef SDL_orbisvideo_h_
#define SDL_orbisvideo_h_

#include "../../SDL_internal.h"
#include "../SDL_sysvideo.h"
#include "../SDL_egl_c.h"

/* The PS4 has exactly one scan-out. Mesa's EGL "orbis" platform sizes every window surface to it
 * (1920x1080 unless ORBIS_EGL_MODE=WxH is set in the environment), and the Vulkan WSI underneath
 * opens sceVideoOut itself - so there is no display handle and no window here, only the EGL surface. */
#define ORBIS_SCREEN_WIDTH  1920
#define ORBIS_SCREEN_HEIGHT 1080
#define ORBIS_REFRESH_RATE  60

typedef struct SDL_WindowData
{
    EGLSurface egl_surface;
    /* Present watchdog (see ORBIS_GL_SwapWindow): swaps on this surface and when the counted ones started. */
    Uint32 swap_count;
    Uint32 swap_window_t0;
    SDL_bool present_checked;
} SDL_WindowData;

#endif /* SDL_orbisvideo_h_ */

/* vi: set ts=4 sw=4 expandtab: */
