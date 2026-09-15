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

/* PlayStation 4 (OpenOrbis) video driver: one fullscreen EGL window on Mesa's EGL "orbis" platform.
 *
 *     SDL_GL_SwapWindow -> eglSwapBuffers -> zink/kopper -> VK_EXT_headless_surface -> wsi_orbis -> flip
 *
 * Facts this driver is built around (mesa-ps4 platform_orbis.c, glrun.c, RetroArch orbis_gl_ctx.c):
 *   - EGL_DEFAULT_DISPLAY selects the orbis platform; Mesa is linked statically (see SDL_egl.c).
 *   - The native window must be NON-NULL - (EGLNativeWindowType)1 - or the EGL core rejects it with
 *     EGL_BAD_NATIVE_WINDOW. The value is otherwise ignored, and the core allows one live surface per
 *     token, which matches the one scan-out.
 *   - The platform only exposes R8G8B8A8 configs (UNORM/sRGB). A B-first config would swap red and
 *     blue silently; since it is never offered, whatever SDL_EGL_ChooseConfig picks is R-first.
 *   - The surface is always the scan-out size (1920x1080; ORBIS_EGL_MODE=WxH overrides in Mesa).
 *   - Swap interval is pinned to 1 (vsync flip); eglSwapInterval cannot change it.
 *   - Desktop GL (EGL_OPENGL_API, core or compat) and GLES both work; there is no libGL, desktop-only
 *     entry points are reached with eglGetProcAddress.
 *   - No mouse, no keyboard, no cursor. The pad is the joystick subsystem's business.
 */

#include "../../SDL_internal.h"

#ifdef SDL_VIDEO_DRIVER_ORBIS

#include <stdarg.h>
#include <orbis_log.h> /* orbis-compat: routed to netlog/klog once ps4_app_init() registered a sink */

#include "SDL_video.h"
#include "SDL_hints.h"
#include "SDL_messagebox.h"
#include "SDL_timer.h"
#include "../SDL_sysvideo.h"
#include "../SDL_egl_c.h"
#include "../../events/SDL_events_c.h"
#include "../../events/SDL_keyboard_c.h"
#include "../../events/SDL_mouse_c.h"
#include "../../events/SDL_windowevents_c.h"

#include "SDL_orbisvideo.h"

#define ORBIS_LOG_PREFIX "S3AIR_SDL_VIDEO: "

/* GL enums used only for the identification log lines, so this file does not need a GL header. */
#define ORBIS_GL_VENDOR                   0x1F00
#define ORBIS_GL_RENDERER                 0x1F01
#define ORBIS_GL_VERSION                  0x1F02
#define ORBIS_GL_SHADING_LANGUAGE_VERSION 0x8B8C

static SDL_Window *orbis_window = NULL; /* the one window that owns the scan-out surface */
static int orbis_swap_interval = 1;
static SDL_bool orbis_logged_gl_strings = SDL_FALSE;
static Uint32 orbis_swap_count = 0;

/* ⚠ ONE SCAN-OUT MEANS A LEAKED CONTEXT IS A DEAD DISPLAY.
 *
 * sceVideoOut allows one open handle per process (a second sceVideoOutOpen fails with
 * ORBIS_VIDEO_OUT_ERROR_RESOURCE_BUSY 0x80290009). wsi_orbis opens it when kopper creates the VkSwapchain
 * (wsi_orbis_scanout_create) and closes it only when that swapchain is destroyed. The swapchain belongs to
 * the kopper displaytarget of the window surface's back buffer, and it is freed when the LAST reference to
 * that resource goes away - eglDestroySurface drops the drawable's, but a GL context that ever rendered to
 * the surface keeps its own (the st_framebuffer in the context's winsys buffer list, purged only when that
 * context is made current again or destroyed). So an application that destroys a GL window without
 * deleting its context keeps the old swapchain - and the video-out handle - alive forever, and every later
 * window surface fails in CreateSwapchainKHR while eglSwapBuffers keeps returning TRUE: no picture, no
 * vsync, no error. Oxygen's render-method switch did exactly that.
 *
 * Hence every context this driver creates is tracked with its window, and destroying the window destroys
 * the contexts the application left behind (before the surface, the order a clean teardown uses). A reaped
 * handle is dead to this driver: SDL_GL_DeleteContext on it is a no-op and SDL_GL_MakeCurrent on it fails -
 * unless a newer context got the same address, which no tracking of raw pointers can tell apart, so an
 * application must still not use a handle after its window is gone (the engine deletes its context first).
 * S3AIR_SDL_ORBIS_KEEP_LEAKED_CONTEXTS=1 turns the reaping off (standalone reproduction test only). */
#define ORBIS_MAX_CONTEXTS 16
static struct
{
    SDL_GLContext context;
    SDL_Window *window;
} orbis_contexts[ORBIS_MAX_CONTEXTS];
static SDL_bool orbis_contexts_overflow = SDL_FALSE; /* table full once: stop refusing untracked handles */

static void ORBIS_Log(const char *fmt, ...)
{
    char line[512];
    va_list ap;

    va_start(ap, fmt);
    SDL_vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);

    /* orbis_log is a no-op until the application registers a sink; SDL_LogDebug is for anything
     * that hooked SDL's own log output instead. */
    orbis_log(ORBIS_LOG_PREFIX "%s", line);
    SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO, ORBIS_LOG_PREFIX "%s", line);
}

static void ORBIS_GetScreenSize(int *w, int *h)
{
    /* Must agree with Mesa's orbis_get_scanout_size(), which reads the same variable. */
    const char *mode = SDL_getenv("ORBIS_EGL_MODE");
    int mw = 0, mh = 0;
    if (mode && SDL_sscanf(mode, "%dx%d", &mw, &mh) == 2 && mw > 0 && mh > 0) {
        *w = mw;
        *h = mh;
    } else {
        *w = ORBIS_SCREEN_WIDTH;
        *h = ORBIS_SCREEN_HEIGHT;
    }
}

static const char *ORBIS_EGLErrorName(EGLint err)
{
    switch (err) {
    case EGL_SUCCESS: return "EGL_SUCCESS";
    case EGL_NOT_INITIALIZED: return "EGL_NOT_INITIALIZED";
    case EGL_BAD_ACCESS: return "EGL_BAD_ACCESS";
    case EGL_BAD_ALLOC: return "EGL_BAD_ALLOC";
    case EGL_BAD_ATTRIBUTE: return "EGL_BAD_ATTRIBUTE";
    case EGL_BAD_CONFIG: return "EGL_BAD_CONFIG";
    case EGL_BAD_CONTEXT: return "EGL_BAD_CONTEXT";
    case EGL_BAD_DISPLAY: return "EGL_BAD_DISPLAY";
    case EGL_BAD_MATCH: return "EGL_BAD_MATCH";
    case EGL_BAD_NATIVE_WINDOW: return "EGL_BAD_NATIVE_WINDOW";
    case EGL_BAD_PARAMETER: return "EGL_BAD_PARAMETER";
    case EGL_BAD_SURFACE: return "EGL_BAD_SURFACE";
    case EGL_CONTEXT_LOST: return "EGL_CONTEXT_LOST";
    default: return "EGL_<other>";
    }
}

/*****************************************************************************/
/* Display                                                                   */
/*****************************************************************************/

static int ORBIS_VideoInit(_THIS)
{
    SDL_VideoDisplay display;
    SDL_DisplayMode mode;

    SDL_zero(mode);
    ORBIS_GetScreenSize(&mode.w, &mode.h);
    mode.refresh_rate = ORBIS_REFRESH_RATE;
    mode.format = SDL_PIXELFORMAT_ABGR8888; /* R8G8B8A8 byte order, what the scan-out really is */
    mode.driverdata = NULL;

    SDL_zero(display);
    display.name = (char *)"PS4 video out";
    display.desktop_mode = mode;
    display.current_mode = mode;
    display.driverdata = NULL;

    if (SDL_AddVideoDisplay(&display, SDL_FALSE) < 0) {
        return -1;
    }

    ORBIS_Log("video init - one display %dx%d@%d", mode.w, mode.h, mode.refresh_rate);
    return 0;
}

static void ORBIS_VideoQuit(_THIS)
{
}

static void ORBIS_GetDisplayModes(_THIS, SDL_VideoDisplay *display)
{
    SDL_AddDisplayMode(display, &display->current_mode);
}

static int ORBIS_SetDisplayMode(_THIS, SDL_VideoDisplay *display, SDL_DisplayMode *mode)
{
    /* There is only the one mode; Mesa/WSI own the video-out configuration. */
    return 0;
}

/*****************************************************************************/
/* OpenGL / EGL                                                              */
/*****************************************************************************/

static int ORBIS_GL_LoadLibrary(_THIS, const char *path)
{
    EGLDisplay dpy;

    if (SDL_EGL_LoadLibrary(_this, path, (NativeDisplayType)EGL_DEFAULT_DISPLAY, 0) < 0) {
        ORBIS_Log("EGL load/initialize FAILED: %s", SDL_GetError());
        return -1;
    }

    dpy = _this->egl_data->egl_display;
    ORBIS_Log("EGL %d.%d up - vendor \"%s\", client APIs \"%s\"",
              _this->egl_data->egl_version_major, _this->egl_data->egl_version_minor,
              _this->egl_data->eglQueryString(dpy, EGL_VENDOR) ? _this->egl_data->eglQueryString(dpy, EGL_VENDOR) : "(null)",
              _this->egl_data->eglQueryString(dpy, EGL_CLIENT_APIS) ? _this->egl_data->eglQueryString(dpy, EGL_CLIENT_APIS) : "(null)");
    return 0;
}

static void ORBIS_GL_UnloadLibrary(_THIS)
{
    if (!_this->egl_data) {
        return;
    }

    /* Keep the EGL display initialized across SDL_RecreateWindow() (the GLES2 renderer does that when
     * it takes over a window): the display is process-wide, eglInitialize() on an initialized display
     * is a no-op, and a terminate/re-initialize cycle of the kopper/wsi_orbis stack has never been run
     * on the console. So only SDL's bookkeeping is released here. */
    _this->egl_data->egl_display = EGL_NO_DISPLAY;
    SDL_EGL_UnloadLibrary(_this);
}

static void *ORBIS_GL_GetProcAddress(_THIS, const char *proc)
{
    void *fn = SDL_EGL_GetProcAddress(_this, proc);
    if (!fn) {
        SDL_SetError("eglGetProcAddress(\"%s\") returned NULL", proc);
    }
    return fn;
}

static void ORBIS_LogChosenConfig(_THIS)
{
    EGLDisplay dpy = _this->egl_data->egl_display;
    EGLConfig cfg = _this->egl_data->egl_config;
    EGLint id = -1, r = -1, g = -1, b = -1, a = -1, d = -1, s = -1, rt = -1;

    _this->egl_data->eglGetConfigAttrib(dpy, cfg, EGL_CONFIG_ID, &id);
    _this->egl_data->eglGetConfigAttrib(dpy, cfg, EGL_RED_SIZE, &r);
    _this->egl_data->eglGetConfigAttrib(dpy, cfg, EGL_GREEN_SIZE, &g);
    _this->egl_data->eglGetConfigAttrib(dpy, cfg, EGL_BLUE_SIZE, &b);
    _this->egl_data->eglGetConfigAttrib(dpy, cfg, EGL_ALPHA_SIZE, &a);
    _this->egl_data->eglGetConfigAttrib(dpy, cfg, EGL_DEPTH_SIZE, &d);
    _this->egl_data->eglGetConfigAttrib(dpy, cfg, EGL_STENCIL_SIZE, &s);
    _this->egl_data->eglGetConfigAttrib(dpy, cfg, EGL_RENDERABLE_TYPE, &rt);
    ORBIS_Log("EGL config id %d: R%d G%d B%d A%d depth %d stencil %d renderable 0x%x (%s)",
              (int)id, (int)r, (int)g, (int)b, (int)a, (int)d, (int)s, (unsigned)rt,
              (r == 8 && g == 8 && b == 8 && a == 8) ? "RGBA8888 - ok" : "NOT RGBA8888");
}

static EGLSurface ORBIS_CreateEGLSurface(_THIS)
{
    /* ⚠ NON-NULL TOKEN: NULL is EGL_BAD_NATIVE_WINDOW in the EGL core. */
    const NativeWindowType token = (NativeWindowType)(uintptr_t)1;
    EGLSurface surface = SDL_EGL_CreateSurface(_this, token);

    if (surface == EGL_NO_SURFACE) {
        /* Ask again with only colour: the orbis platform is deliberately sparse (no MSAA configs, and
         * which depth/stencil formats exist depends on what zink finds in RADV). The engine renders into
         * its own FBOs, so the window buffer rarely needs more. */
        const int depth = _this->gl_config.depth_size;
        const int stencil = _this->gl_config.stencil_size;
        const int msb = _this->gl_config.multisamplebuffers;
        const int mss = _this->gl_config.multisamplesamples;
        const int srgb = _this->gl_config.framebuffer_srgb_capable;
        const EGLint err = _this->egl_data->eglGetError();

        ORBIS_Log("eglCreateWindowSurface with depth %d stencil %d samples %d/%d failed (%s: %s) - retrying colour-only",
                  depth, stencil, msb, mss, ORBIS_EGLErrorName(err), SDL_GetError());
        _this->gl_config.depth_size = 0;
        _this->gl_config.stencil_size = 0;
        _this->gl_config.multisamplebuffers = 0;
        _this->gl_config.multisamplesamples = 0;
        _this->gl_config.framebuffer_srgb_capable = 0;
        surface = SDL_EGL_CreateSurface(_this, token);
        _this->gl_config.depth_size = depth;
        _this->gl_config.stencil_size = stencil;
        _this->gl_config.multisamplebuffers = msb;
        _this->gl_config.multisamplesamples = mss;
        _this->gl_config.framebuffer_srgb_capable = srgb;
    }

    if (surface == EGL_NO_SURFACE) {
        ORBIS_Log("eglCreateWindowSurface FAILED: %s", SDL_GetError());
        return EGL_NO_SURFACE;
    }

    ORBIS_LogChosenConfig(_this);
    {
        EGLint sw = -1, sh = -1;
        eglQuerySurface(_this->egl_data->egl_display, surface, EGL_WIDTH, &sw);
        eglQuerySurface(_this->egl_data->egl_display, surface, EGL_HEIGHT, &sh);
        ORBIS_Log("window surface %dx%d created (%s API requested)", (int)sw, (int)sh,
                  _this->gl_config.profile_mask == SDL_GL_CONTEXT_PROFILE_ES ? "GLES" : "desktop GL");
    }
    return surface;
}

static void ORBIS_LogGLStrings(_THIS)
{
    typedef const unsigned char *(*GetStringFn)(unsigned int);
    GetStringFn getString;

    if (orbis_logged_gl_strings) {
        return;
    }
    getString = (GetStringFn)SDL_EGL_GetProcAddress(_this, "glGetString");
    if (!getString) {
        ORBIS_Log("glGetString not resolvable through eglGetProcAddress");
        return;
    }
    orbis_logged_gl_strings = SDL_TRUE;
    ORBIS_Log("GL_VENDOR   = %s", getString(ORBIS_GL_VENDOR) ? (const char *)getString(ORBIS_GL_VENDOR) : "(null)");
    ORBIS_Log("GL_RENDERER = %s", getString(ORBIS_GL_RENDERER) ? (const char *)getString(ORBIS_GL_RENDERER) : "(null)");
    ORBIS_Log("GL_VERSION  = %s", getString(ORBIS_GL_VERSION) ? (const char *)getString(ORBIS_GL_VERSION) : "(null)");
    ORBIS_Log("GLSL        = %s", getString(ORBIS_GL_SHADING_LANGUAGE_VERSION) ? (const char *)getString(ORBIS_GL_SHADING_LANGUAGE_VERSION) : "(null)");
}

static int ORBIS_FindContext(SDL_GLContext context)
{
    int i;
    for (i = 0; context && i < ORBIS_MAX_CONTEXTS; ++i) {
        if (orbis_contexts[i].context == context) {
            return i;
        }
    }
    return -1;
}

static void ORBIS_TrackContext(SDL_GLContext context, SDL_Window *window)
{
    int i;
    for (i = 0; i < ORBIS_MAX_CONTEXTS; ++i) {
        if (!orbis_contexts[i].context) {
            orbis_contexts[i].context = context;
            orbis_contexts[i].window = window;
            return;
        }
    }
    if (!orbis_contexts_overflow) {
        ORBIS_Log("more than %d live GL contexts - context tracking degraded (leaked contexts are no longer reaped)", ORBIS_MAX_CONTEXTS);
    }
    orbis_contexts_overflow = SDL_TRUE;
}

static SDL_bool ORBIS_IsTrackedContext(SDL_GLContext context)
{
    return (orbis_contexts_overflow || ORBIS_FindContext(context) >= 0) ? SDL_TRUE : SDL_FALSE;
}

static SDL_GLContext ORBIS_GL_CreateContext(_THIS, SDL_Window *window)
{
    SDL_WindowData *data = (SDL_WindowData *)window->driverdata;
    SDL_GLContext ctx;
    const int req_major = _this->gl_config.major_version;
    const int req_minor = _this->gl_config.minor_version;
    const int req_profile = _this->gl_config.profile_mask;

    if (!data || data->egl_surface == EGL_NO_SURFACE) {
        SDL_SetError("orbis: window has no EGL surface");
        return NULL;
    }

    ctx = SDL_EGL_CreateContext(_this, data->egl_surface);

    if (!ctx && req_profile != SDL_GL_CONTEXT_PROFILE_ES) {
        /* Desktop GL fallback rungs, as in RetroArch's orbis_gl_ctx.c: a refusal costs one
         * eglCreateContext, a black screen costs a console round trip. */
        static const struct { int major, minor, profile; } rungs[] = {
            { 3, 3, SDL_GL_CONTEXT_PROFILE_CORE },
            { 3, 2, SDL_GL_CONTEXT_PROFILE_CORE },
            { 3, 3, 0 },
            { 2, 1, 0 },
        };
        int i;
        ORBIS_Log("GL %d.%d profile 0x%x refused (%s) - trying fallback versions",
                  req_major, req_minor, (unsigned)req_profile, SDL_GetError());
        for (i = 0; !ctx && i < (int)SDL_arraysize(rungs); ++i) {
            _this->gl_config.major_version = rungs[i].major;
            _this->gl_config.minor_version = rungs[i].minor;
            _this->gl_config.profile_mask = rungs[i].profile;
            ctx = SDL_EGL_CreateContext(_this, data->egl_surface);
            if (!ctx) {
                ORBIS_Log("GL %d.%d profile 0x%x refused (%s)", rungs[i].major, rungs[i].minor,
                          (unsigned)rungs[i].profile, SDL_GetError());
            }
        }
        _this->gl_config.major_version = req_major;
        _this->gl_config.minor_version = req_minor;
        _this->gl_config.profile_mask = req_profile;
    }

    if (!ctx) {
        ORBIS_Log("eglCreateContext FAILED: %s", SDL_GetError());
        return NULL;
    }

    ORBIS_Log("context created (requested %s %d.%d, profile mask 0x%x)",
              req_profile == SDL_GL_CONTEXT_PROFILE_ES ? "GLES" : "GL", req_major, req_minor, (unsigned)req_profile);
    ORBIS_TrackContext(ctx, window);
    ORBIS_LogGLStrings(_this);
    return ctx;
}

static int ORBIS_GL_MakeCurrent(_THIS, SDL_Window *window, SDL_GLContext context)
{
    EGLSurface surface = EGL_NO_SURFACE;
    if (context && !ORBIS_IsTrackedContext(context)) {
        return SDL_SetError("orbis: GL context %p was destroyed together with its window", (void *)context);
    }
    if (window && context && window->driverdata) {
        surface = ((SDL_WindowData *)window->driverdata)->egl_surface;
    }
    return SDL_EGL_MakeCurrent(_this, surface, context);
}

static int ORBIS_GL_SetSwapInterval(_THIS, int interval)
{
    /* Accepted and ignored: the flip is vsync-locked and the WSI throttles the producer, so the
     * effective interval is always 1 (Mesa logs this once itself). Not calling eglSwapInterval keeps
     * a vsync-off request from turning into an error. */
    if (interval != 1) {
        static SDL_bool said = SDL_FALSE;
        if (!said) {
            said = SDL_TRUE;
            ORBIS_Log("swap interval %d requested - pinned at 1 on this platform", interval);
        }
    }
    orbis_swap_interval = 1;
    return 0;
}

static int ORBIS_GL_GetSwapInterval(_THIS)
{
    return orbis_swap_interval;
}

static int ORBIS_GL_SwapWindow(_THIS, SDL_Window *window)
{
    SDL_WindowData *data = (SDL_WindowData *)window->driverdata;
    int ret;

    if (!data || data->egl_surface == EGL_NO_SURFACE) {
        return SDL_SetError("orbis: window has no EGL surface");
    }

    ret = SDL_EGL_SwapBuffers(_this, data->egl_surface);
    ++orbis_swap_count;
    if (ret < 0) {
        static Uint32 failures = 0;
        if (failures++ < 5) {
            ORBIS_Log("eglSwapBuffers failed at swap %u: %s", (unsigned)orbis_swap_count, SDL_GetError());
        }
    } else if (orbis_swap_count == 1) {
        ORBIS_Log("first eglSwapBuffers OK");
    }

    /* ⚠ A MISSING SWAPCHAIN DOES NOT FAIL THE SWAP. When kopper cannot create the swapchain (klog:
     * "wsi/orbis: sceVideoOutOpen -> ..." / "zink: could not create swapchain") eglSwapBuffers still returns
     * TRUE and simply stops blocking on the vsync-locked flip. So time a run of swaps on every new surface:
     * a present that really reaches the display cannot run faster than ~60 per second. One line per surface. */
    if (ret == 0) {
        ++data->swap_count;
    }
    if (ret == 0 && !data->present_checked) {
        const Uint32 first = 10, swaps = 120;
        if (data->swap_count == first) {
            data->swap_window_t0 = SDL_GetTicks();
        } else if (data->swap_count == first + swaps) {
            const Uint32 ms = SDL_GetTicks() - data->swap_window_t0;
            data->present_checked = SDL_TRUE;
            if (ms < swaps * 8) {
                ORBIS_Log("PRESENT NOT VSYNC-LOCKED on this window surface: %u swaps in %u ms - the swapchain is probably "
                          "missing (klog: wsi/orbis sceVideoOutOpen / zink could not create swapchain), nothing reaches the screen",
                          (unsigned)swaps, (unsigned)ms);
            } else {
                ORBIS_Log("present vsync-locked on this window surface: %u swaps in %u ms", (unsigned)swaps, (unsigned)ms);
            }
        }
    }
    return ret;
}

static void ORBIS_GL_DeleteContext(_THIS, SDL_GLContext context)
{
    const int index = ORBIS_FindContext(context);

    if (index < 0 && !orbis_contexts_overflow) {
        /* Already destroyed together with its window (see orbis_contexts): the handle may name a context
         * Mesa has allocated again since, so it must not reach eglDestroyContext. */
        ORBIS_Log("SDL_GL_DeleteContext(%p): already destroyed with its window - ignored", (void *)context);
        return;
    }
    if (index >= 0) {
        orbis_contexts[index].context = NULL;
        orbis_contexts[index].window = NULL;
    }
    SDL_EGL_DeleteContext(_this, context);
    ORBIS_Log("context %p deleted", (void *)context);
}

/* Destroys the GL contexts created for `window` that the application has not deleted. Runs while the
 * window's EGL surface still exists and the EGL library is still loaded (SDL_DestroyWindow unloads it
 * right after DestroyWindow). Destroying the context releases its framebuffer's back-buffer references
 * synchronously (zink waits for the queue and clears its batch states), so together with the surface
 * destroyed next, the swapchain - and sceVideoOut - is gone before the next window asks for one. */
static void ORBIS_ReapLeakedContexts(_THIS, SDL_Window *window)
{
    int i;
    const char *keep = SDL_getenv("S3AIR_SDL_ORBIS_KEEP_LEAKED_CONTEXTS");
    const SDL_bool keep_leaked = (keep && *keep && *keep != '0') ? SDL_TRUE : SDL_FALSE;

    for (i = 0; i < ORBIS_MAX_CONTEXTS; ++i) {
        SDL_GLContext context = orbis_contexts[i].context;
        if (!context || orbis_contexts[i].window != window) {
            continue;
        }
        if (keep_leaked) {
            ORBIS_Log("window destroyed with GL context %p still alive - NOT reaped (S3AIR_SDL_ORBIS_KEEP_LEAKED_CONTEXTS): "
                      "its swapchain keeps sceVideoOut open", (void *)context);
            orbis_contexts[i].window = NULL; /* stays a valid, tracked handle */
            continue;
        }
        ORBIS_Log("window destroyed with GL context %p still alive - destroying it now (one scan-out: its swapchain "
                  "would keep sceVideoOut open and every later window would show nothing)", (void *)context);
        if (SDL_GL_GetCurrentContext() == context) {
            SDL_GL_MakeCurrent(NULL, NULL);
        }
        if (_this->egl_data && eglGetCurrentContext() == (EGLContext)context) {
            _this->egl_data->eglMakeCurrent(_this->egl_data->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        }
        orbis_contexts[i].context = NULL;
        orbis_contexts[i].window = NULL;
        SDL_EGL_DeleteContext(_this, context);
    }
}

/*****************************************************************************/
/* Windows                                                                   */
/*****************************************************************************/

static int ORBIS_CreateWindow(_THIS, SDL_Window *window)
{
    SDL_WindowData *data;
    int w, h;

    if (orbis_window && orbis_window != window) {
        return SDL_SetError("orbis: only one window is supported (one scan-out)");
    }

    data = (SDL_WindowData *)SDL_calloc(1, sizeof(SDL_WindowData));
    if (!data) {
        return SDL_OutOfMemory();
    }
    data->egl_surface = EGL_NO_SURFACE;

    /* Always the whole screen, whatever was asked for. */
    ORBIS_GetScreenSize(&w, &h);
    window->x = 0;
    window->y = 0;
    window->w = w;
    window->h = h;
    window->flags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS;
    window->flags &= ~SDL_WINDOW_RESIZABLE;

    if (window->flags & SDL_WINDOW_OPENGL) {
        data->egl_surface = ORBIS_CreateEGLSurface(_this);
        if (data->egl_surface == EGL_NO_SURFACE) {
            SDL_free(data);
            return -1; /* error already set by SDL_EGL_CreateSurface */
        }
    }

    window->driverdata = data;
    orbis_window = window;

    /* There is no window system to hand out focus, so the one window has it from the start. */
    SDL_SetMouseFocus(window);
    SDL_SetKeyboardFocus(window);

    ORBIS_Log("window created %dx%d (%s)", w, h, (window->flags & SDL_WINDOW_OPENGL) ? "OpenGL" : "no GL surface");
    return 0;
}

static int ORBIS_CreateWindowFrom(_THIS, SDL_Window *window, const void *data)
{
    return SDL_Unsupported();
}

static void ORBIS_ShowWindow(_THIS, SDL_Window *window)
{
    SDL_SetMouseFocus(window);
    SDL_SetKeyboardFocus(window);
    SDL_SendWindowEvent(window, SDL_WINDOWEVENT_EXPOSED, 0, 0);
}

static void ORBIS_HideWindow(_THIS, SDL_Window *window)
{
}

static void ORBIS_SetWindowFullscreen(_THIS, SDL_Window *window, SDL_VideoDisplay *display, SDL_bool fullscreen)
{
    /* Always fullscreen. */
}

static void ORBIS_DestroyWindow(_THIS, SDL_Window *window)
{
    SDL_WindowData *data = (SDL_WindowData *)window->driverdata;

    ORBIS_ReapLeakedContexts(_this, window);

    if (data) {
        if (data->egl_surface != EGL_NO_SURFACE && _this->egl_data) {
            /* A surface that is still current is only marked for deletion by EGL, and the core's
             * one-surface-per-native-token rule would then refuse the next window (token 1) with
             * EGL_BAD_ALLOC - SDL_RecreateWindow does not release the context first, so do it here. */
            if (eglGetCurrentSurface(EGL_DRAW) == data->egl_surface) {
                _this->egl_data->eglMakeCurrent(_this->egl_data->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            }
            SDL_EGL_DestroySurface(_this, data->egl_surface);
            ORBIS_Log("window destroyed (EGL surface released after %u swap(s))", (unsigned)data->swap_count);
        } else {
            ORBIS_Log("window destroyed (no GL surface)");
        }
        SDL_free(data);
        window->driverdata = NULL;
    }
    if (orbis_window == window) {
        orbis_window = NULL;
    }
}

/*****************************************************************************/
/* App lifecycle: PS button / system UI overlay / background execution       */
/*****************************************************************************/

/* ⚠ NOT FROM THE SDK HEADER: OpenOrbis 0.5.4's <orbis/SystemService.h> declares both functions as
 * `void f()` and has no status/event types. Layouts as in Sony's SDK (SceSystemServiceStatus: eventNum +
 * five bools + 125 reserved bytes; SceSystemServiceEvent: eventType + an 8192-byte parameter union), with
 * extra tail room so a layout mistake cannot write past the buffer. Bound through asm labels so these
 * prototypes never clash with the header's. libSceSystemService is already linked (stubs export both).
 *
 * What the system tells an application (and what it does not):
 *   - PS button / quick menu: the system UI is overlaid and the app keeps running with the pad taken away
 *     -> isSystemUiOverlaid / isInBackgroundExecution. Mapped to SDL_WINDOWEVENT_FOCUS_LOST / _GAINED
 *     through the keyboard focus (the engine auto-pauses, and saves its settings, on focus loss).
 *   - Another app started / rest mode: the process is suspended outright; afterwards the event queue
 *     carries SCE_SYSTEM_SERVICE_EVENT_ON_RESUME (logged, plus the stall of the main loop).
 *   - "Close application": the process is killed, there is NO event to react to - which is why state is
 *     saved when focus is lost (closing always goes through the overlaid system UI first). */
typedef struct
{
    int32_t eventNum;
    uint8_t isSystemUiOverlaid;
    uint8_t isInBackgroundExecution;
    uint8_t isCpuMode7CpuNormal;
    uint8_t isGameLiveStreamingOnAir;
    uint8_t isOutOfVrPlayArea;
    uint8_t reserved[125];
    uint8_t guard[256];
} ORBIS_SystemServiceStatus;

typedef struct
{
    int32_t eventType;
    uint8_t data[8192];
    uint8_t guard[256];
} ORBIS_SystemServiceEvent;

#define ORBIS_SYSTEM_SERVICE_EVENT_ON_RESUME 0x10000000

extern int32_t ORBIS_sceSystemServiceGetStatus(ORBIS_SystemServiceStatus *status) __asm__("sceSystemServiceGetStatus");
extern int32_t ORBIS_sceSystemServiceReceiveEvent(ORBIS_SystemServiceEvent *event) __asm__("sceSystemServiceReceiveEvent");

#define ORBIS_LIFECYCLE_POLL_MS  100  /* status is IPC to the shell; ~10 polls per second are plenty */
#define ORBIS_LIFECYCLE_STALL_MS 1500 /* a longer gap between pumps = the process was suspended (or hung) */

static SDL_bool orbis_lifecycle_disabled = SDL_FALSE;
static SDL_bool orbis_lifecycle_have_state = SDL_FALSE;
static SDL_bool orbis_lifecycle_focused = SDL_TRUE;
static Uint32 orbis_lifecycle_last_poll = 0;
static Uint32 orbis_lifecycle_last_pump = 0;
static int orbis_lifecycle_failures = 0;

static void ORBIS_LifecycleLog(const char *fmt, ...)
{
    char line[256];
    va_list ap;

    va_start(ap, fmt);
    SDL_vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    orbis_log("S3AIR_LIFECYCLE: %s", line);
}

static const char *ORBIS_EventName(int32_t type)
{
    switch (type) {
    case 0x10000000: return "ON_RESUME";
    case 0x10000001: return "GAME_LIVE_STREAMING_STATUS_UPDATE";
    case 0x10000002: return "SESSION_INVITATION";
    case 0x10000003: return "ENTITLEMENT_UPDATE";
    case 0x10000004: return "GAME_CUSTOM_DATA";
    case 0x10000005: return "DISPLAY_SAFE_AREA_UPDATE";
    case 0x10000006: return "URL_OPEN";
    case 0x10000007: return "LAUNCH_APP";
    case 0x10000008: return "APP_LAUNCH_LINK";
    case 0x10000009: return "ADDCONTENT_INSTALL";
    case 0x1000000c: return "PLAYGO_LOCUS_UPDATE";
    default: return "other";
    }
}

static void ORBIS_PollLifecycle(void)
{
    static ORBIS_SystemServiceStatus status; /* main thread only; static keeps 8 KiB off the stack */
    static ORBIS_SystemServiceEvent event;
    const Uint32 now = SDL_GetTicks();
    SDL_bool focused;
    int32_t rc;

    if (orbis_lifecycle_last_pump != 0 && (Uint32)(now - orbis_lifecycle_last_pump) > ORBIS_LIFECYCLE_STALL_MS) {
        ORBIS_LifecycleLog("main loop was stalled for %u ms (suspended?)", (unsigned)(now - orbis_lifecycle_last_pump));
        orbis_lifecycle_last_poll = 0; /* poll right away */
    }
    orbis_lifecycle_last_pump = now;

    if (orbis_lifecycle_disabled) {
        return;
    }
    if (orbis_lifecycle_last_poll != 0 && (Uint32)(now - orbis_lifecycle_last_poll) < ORBIS_LIFECYCLE_POLL_MS) {
        return;
    }
    orbis_lifecycle_last_poll = now ? now : 1;

    SDL_zero(status);
    rc = ORBIS_sceSystemServiceGetStatus(&status);
    if (rc < 0) {
        if (++orbis_lifecycle_failures <= 3) {
            ORBIS_LifecycleLog("sceSystemServiceGetStatus rc=0x%08x", (unsigned)rc);
        }
        if (orbis_lifecycle_failures >= 20) {
            ORBIS_LifecycleLog("sceSystemServiceGetStatus keeps failing - lifecycle polling disabled");
            orbis_lifecycle_disabled = SDL_TRUE;
        }
        return;
    }
    orbis_lifecycle_failures = 0;

    /* The queue is drained so it cannot fill up; nothing in it asks the application to quit. */
    if (status.eventNum > 0) {
        int i;
        for (i = 0; i < status.eventNum && i < 8; ++i) {
            SDL_zero(event);
            rc = ORBIS_sceSystemServiceReceiveEvent(&event);
            if (rc < 0) {
                ORBIS_LifecycleLog("sceSystemServiceReceiveEvent rc=0x%08x", (unsigned)rc);
                break;
            }
            ORBIS_LifecycleLog("system event 0x%08x (%s)", (unsigned)event.eventType, ORBIS_EventName(event.eventType));
        }
    }

    focused = (status.isSystemUiOverlaid || status.isInBackgroundExecution) ? SDL_FALSE : SDL_TRUE;
    if (!orbis_lifecycle_have_state) {
        orbis_lifecycle_have_state = SDL_TRUE;
        ORBIS_LifecycleLog("status polling up: system UI overlaid %d, background execution %d, events %d",
                           (int)status.isSystemUiOverlaid, (int)status.isInBackgroundExecution, (int)status.eventNum);
    }
    if (focused == orbis_lifecycle_focused) {
        return;
    }
    orbis_lifecycle_focused = focused;

    ORBIS_LifecycleLog("%s (system UI overlaid %d, background execution %d)", focused ? "focus gained" : "focus lost",
                       (int)status.isSystemUiOverlaid, (int)status.isInBackgroundExecution);
    if (orbis_window) {
        /* SDL_SetKeyboardFocus sends SDL_WINDOWEVENT_FOCUS_LOST / _GAINED to the window. No minimize on
         * focus loss: the device has VIDEO_DEVICE_QUIRK_DISABLE_DISPLAY_MODE_SWITCHING. */
        SDL_SetKeyboardFocus(focused ? orbis_window : NULL);
    }
}

static void ORBIS_PumpEvents(_THIS)
{
    /* No window system events exist; the pad is polled by the joystick subsystem. Only the system
     * service status is watched here. */
    ORBIS_PollLifecycle();
}

/*****************************************************************************/
/* Message boxes: no system dialog is wired up; log the text and pick the default button. */
/*****************************************************************************/

static int ORBIS_ShowMessageBoxImpl(const SDL_MessageBoxData *messageboxdata, int *buttonid)
{
    int i;
    int chosen = -1;

    orbis_log(ORBIS_LOG_PREFIX "message box \"%s\": %s",
              messageboxdata->title ? messageboxdata->title : "",
              messageboxdata->message ? messageboxdata->message : "");
    for (i = 0; i < messageboxdata->numbuttons; ++i) {
        if (messageboxdata->buttons[i].flags & SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT) {
            chosen = messageboxdata->buttons[i].buttonid;
            break;
        }
    }
    if (chosen < 0 && messageboxdata->numbuttons > 0) {
        chosen = messageboxdata->buttons[0].buttonid;
    }
    if (buttonid) {
        *buttonid = chosen;
    }
    return 0;
}

static int ORBIS_ShowMessageBox(_THIS, const SDL_MessageBoxData *messageboxdata, int *buttonid)
{
    return ORBIS_ShowMessageBoxImpl(messageboxdata, buttonid);
}

/*****************************************************************************/
/* Bootstrap                                                                 */
/*****************************************************************************/

static void ORBIS_DeleteDevice(SDL_VideoDevice *device)
{
    SDL_free(device);
}

static SDL_VideoDevice *ORBIS_CreateDevice(void)
{
    SDL_VideoDevice *device = (SDL_VideoDevice *)SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (!device) {
        SDL_OutOfMemory();
        return NULL;
    }

    device->free = ORBIS_DeleteDevice;
    device->quirk_flags = VIDEO_DEVICE_QUIRK_FULLSCREEN_ONLY | VIDEO_DEVICE_QUIRK_DISABLE_DISPLAY_MODE_SWITCHING;

    device->VideoInit = ORBIS_VideoInit;
    device->VideoQuit = ORBIS_VideoQuit;
    device->GetDisplayModes = ORBIS_GetDisplayModes;
    device->SetDisplayMode = ORBIS_SetDisplayMode;

    device->CreateSDLWindow = ORBIS_CreateWindow;
    device->CreateSDLWindowFrom = ORBIS_CreateWindowFrom;
    device->ShowWindow = ORBIS_ShowWindow;
    device->HideWindow = ORBIS_HideWindow;
    device->SetWindowFullscreen = ORBIS_SetWindowFullscreen;
    device->DestroyWindow = ORBIS_DestroyWindow;

    /* No CreateWindowFramebuffer: SDL_GetWindowSurface() falls back to a streaming texture on the
     * GLES2 renderer (SDL_VIDEO_RENDER_OGL_ES2), which recreates the window with SDL_WINDOW_OPENGL. */

    device->GL_LoadLibrary = ORBIS_GL_LoadLibrary;
    device->GL_GetProcAddress = ORBIS_GL_GetProcAddress;
    device->GL_UnloadLibrary = ORBIS_GL_UnloadLibrary;
    device->GL_CreateContext = ORBIS_GL_CreateContext;
    device->GL_MakeCurrent = ORBIS_GL_MakeCurrent;
    device->GL_SetSwapInterval = ORBIS_GL_SetSwapInterval;
    device->GL_GetSwapInterval = ORBIS_GL_GetSwapInterval;
    device->GL_SwapWindow = ORBIS_GL_SwapWindow;
    device->GL_DeleteContext = ORBIS_GL_DeleteContext;

    device->ShowMessageBox = ORBIS_ShowMessageBox;
    device->PumpEvents = ORBIS_PumpEvents;

    return device;
}

VideoBootStrap ORBIS_bootstrap = {
    "orbis",
    "PS4 EGL video driver (Mesa zink/RADV)",
    ORBIS_CreateDevice,
    ORBIS_ShowMessageBoxImpl
};

#endif /* SDL_VIDEO_DRIVER_ORBIS */

/* vi: set ts=4 sw=4 expandtab: */
