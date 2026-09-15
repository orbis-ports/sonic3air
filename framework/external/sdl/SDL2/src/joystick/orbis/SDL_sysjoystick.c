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

#ifdef SDL_JOYSTICK_ORBIS

/* PS4 (Orbis) DualShock 4 through libScePad.
 *
 * Hardware facts (RetroArch input/drivers_joypad/ps4_joypad.c, Tempest ps4api.cpp):
 *  - scePadOpen needs the REAL logged-in user id; the 0xFF system user is refused
 *    with 0x809b0001 on hardware (the emulator accepts it).
 *  - ALREADY_OPENED is not a failure: scePadGetHandle returns the existing handle.
 *  - A sleeping/disconnected pad reports connected == 0; the handle stays valid and
 *    the pad comes back on the same handle, so a slot is never written off - it is
 *    read every update and reported as "everything released" while disconnected.
 *
 * One SDL joystick per logged-in user at init time (initial user first, up to 4).
 * Handles are cached for the life of the process and never closed, so an SDL
 * joystick subsystem restart reopens nothing.
 *
 * Layout (matches the SDL_gamecontrollerdb.h entry under SDL_JOYSTICK_ORBIS):
 *   buttons: 0 Cross, 1 Circle, 2 Square, 3 Triangle, 4 L1, 5 R1, 6 L3, 7 R3,
 *            8 Options, 9 Touchpad click, 10 L2 (digital), 11 R2 (digital)
 *   axes:    0 LX, 1 LY, 2 RX, 3 RY (-32768..32767, up/left negative),
 *            4 L2, 5 R2 (-32768 released .. 32767 fully pressed)
 *   hat 0:   D-pad
 * The Share and PS buttons are reserved by the system and never reach scePad.
 */

#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_joystick.h"
#include "SDL_log.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"
#include "../usb_ids.h"

#include <stdarg.h>
#include <stdint.h>

#include <orbis/Pad.h>
#include <orbis/UserService.h>

/* orbis-compat: routed to the UDP netlog once ps4_app_init() has run. */
#include <orbis_log.h>

#define ORBIS_PAD_NAME       "PS4 Controller"
#define ORBIS_PAD_MAX        ORBIS_USER_SERVICE_MAX_LOGIN_USERS
#define ORBIS_PAD_NBUTTONS   12
#define ORBIS_PAD_NAXES      6

/* The Bluetooth bus and a DualShock 4 VID/PID in the GUID make
   SDL_GameControllerGetType() report SDL_CONTROLLER_TYPE_PS4. The GUID string is
   "050000004c050000c405000000000000" (CRC field zeroed, as the database expects). */
#define ORBIS_PAD_BUS        SDL_HARDWARE_BUS_BLUETOOTH
#define ORBIS_PAD_VENDOR     USB_VENDOR_SONY
#define ORBIS_PAD_PRODUCT    USB_PRODUCT_SONY_DS4

static const uint32_t orbis_button_map[ORBIS_PAD_NBUTTONS] = {
    ORBIS_PAD_BUTTON_CROSS,
    ORBIS_PAD_BUTTON_CIRCLE,
    ORBIS_PAD_BUTTON_SQUARE,
    ORBIS_PAD_BUTTON_TRIANGLE,
    ORBIS_PAD_BUTTON_L1,
    ORBIS_PAD_BUTTON_R1,
    ORBIS_PAD_BUTTON_L3,
    ORBIS_PAD_BUTTON_R3,
    ORBIS_PAD_BUTTON_OPTIONS,
    ORBIS_PAD_BUTTON_TOUCH_PAD,
    ORBIS_PAD_BUTTON_L2,
    ORBIS_PAD_BUTTON_R2
};

typedef struct OrbisPadSlot
{
    int32_t user_id;
    int32_t handle;
    SDL_JoystickID instance_id;
    SDL_bool connected;      /* as last reported by scePadReadState */
    SDL_bool read_failing;   /* last scePadReadState returned an error */
    OrbisPadVibeParam vibe;
} OrbisPadSlot;

static OrbisPadSlot orbis_pads[ORBIS_PAD_MAX];
static int orbis_num_pads = 0;
static SDL_bool orbis_pad_lib_up = SDL_FALSE;

static void ORBISPAD_Log(const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(1);
static void ORBISPAD_Log(const char *fmt, ...)
{
    char line[256];
    va_list ap;

    va_start(ap, fmt);
    SDL_vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);

    if (orbis_log_enabled()) {
        orbis_log("S3AIR_SDL_PAD: %s", line);
    } else {
        SDL_Log("S3AIR_SDL_PAD: %s", line);
    }
}

static int32_t ORBISPAD_OpenForUser(int32_t user_id)
{
    int32_t handle = scePadOpen(user_id, ORBIS_PAD_PORT_TYPE_STANDARD, 0, NULL);
    if (handle == (int32_t)ORBIS_PAD_ERROR_ALREADY_OPENED) {
        const int32_t existing = scePadGetHandle(user_id, ORBIS_PAD_PORT_TYPE_STANDARD, 0);
        ORBISPAD_Log("init: scePadOpen(user=0x%08x) ALREADY_OPENED, scePadGetHandle rc=0x%08x",
                     (unsigned)user_id, (unsigned)existing);
        handle = existing;
    } else {
        ORBISPAD_Log("init: scePadOpen(user=0x%08x, STANDARD, 0, NULL) rc=0x%08x",
                     (unsigned)user_id, (unsigned)handle);
    }
    return handle;
}

static void ORBISPAD_AddSlot(int32_t user_id)
{
    int i;
    int32_t handle;

    if (user_id == ORBIS_USER_SERVICE_USER_ID_INVALID || orbis_num_pads >= ORBIS_PAD_MAX) {
        return;
    }
    for (i = 0; i < orbis_num_pads; i++) {
        if (orbis_pads[i].user_id == user_id) {
            return;
        }
    }

    handle = ORBISPAD_OpenForUser(user_id);
    if (handle < 0) {
        return;
    }

    SDL_zero(orbis_pads[orbis_num_pads]);
    orbis_pads[orbis_num_pads].user_id = user_id;
    orbis_pads[orbis_num_pads].handle = handle;
    orbis_pads[orbis_num_pads].connected = SDL_TRUE;
    orbis_num_pads++;
}

static int ORBISPAD_JoystickInit(void)
{
    OrbisUserServiceInitializeParams params;
    OrbisUserServiceLoginUserIdList users;
    int32_t user_rc, pad_rc, initial_rc, list_rc;
    int32_t initial_user = ORBIS_USER_SERVICE_USER_ID_INVALID;
    int i;

    /* Handles survive an SDL joystick subsystem restart; only open once. */
    if (!orbis_pad_lib_up) {
        SDL_zero(params);
        params.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
        user_rc = sceUserServiceInitialize(&params);
        pad_rc = scePadInit();

        initial_rc = sceUserServiceGetInitialUser(&initial_user);
        ORBISPAD_Log("init: sceUserServiceInitialize rc=0x%08x, scePadInit rc=0x%08x, "
                     "sceUserServiceGetInitialUser rc=0x%08x user=0x%08x",
                     (unsigned)user_rc, (unsigned)pad_rc, (unsigned)initial_rc, (unsigned)initial_user);

        if (initial_rc >= 0) {
            ORBISPAD_AddSlot(initial_user);
        }

        SDL_zero(users);
        list_rc = sceUserServiceGetLoginUserIdList(&users);
        ORBISPAD_Log("init: sceUserServiceGetLoginUserIdList rc=0x%08x users=0x%08x 0x%08x 0x%08x 0x%08x",
                     (unsigned)list_rc, (unsigned)users.userId[0], (unsigned)users.userId[1],
                     (unsigned)users.userId[2], (unsigned)users.userId[3]);
        if (list_rc >= 0) {
            for (i = 0; i < ORBIS_USER_SERVICE_MAX_LOGIN_USERS; i++) {
                ORBISPAD_AddSlot(users.userId[i]);
            }
        }

        orbis_pad_lib_up = SDL_TRUE;
    }

    for (i = 0; i < orbis_num_pads; i++) {
        orbis_pads[i].instance_id = SDL_GetNextJoystickInstanceID();
        SDL_PrivateJoystickAdded(orbis_pads[i].instance_id);
    }

    ORBISPAD_Log("init: %d pad(s) exposed as SDL joysticks", orbis_num_pads);
    return 0;
}

static int ORBISPAD_JoystickGetCount(void)
{
    return orbis_num_pads;
}

static void ORBISPAD_JoystickDetect(void)
{
}

static const char *ORBISPAD_JoystickGetDeviceName(int device_index)
{
    if (device_index < 0 || device_index >= orbis_num_pads) {
        SDL_SetError("No joystick available with that index");
        return NULL;
    }
    return ORBIS_PAD_NAME;
}

static const char *ORBISPAD_JoystickGetDevicePath(int device_index)
{
    return NULL;
}

static int ORBISPAD_JoystickGetDeviceSteamVirtualGamepadSlot(int device_index)
{
    return -1;
}

static int ORBISPAD_JoystickGetDevicePlayerIndex(int device_index)
{
    return device_index;
}

static void ORBISPAD_JoystickSetDevicePlayerIndex(int device_index, int player_index)
{
}

static SDL_JoystickGUID ORBISPAD_JoystickGetDeviceGUID(int device_index)
{
    return SDL_CreateJoystickGUID(ORBIS_PAD_BUS, ORBIS_PAD_VENDOR, ORBIS_PAD_PRODUCT, 0,
                                  NULL, ORBIS_PAD_NAME, 0, 0);
}

static SDL_JoystickID ORBISPAD_JoystickGetDeviceInstanceID(int device_index)
{
    if (device_index < 0 || device_index >= orbis_num_pads) {
        return -1;
    }
    return orbis_pads[device_index].instance_id;
}

static OrbisPadSlot *ORBISPAD_SlotFor(SDL_Joystick *joystick)
{
    return (OrbisPadSlot *)joystick->hwdata;
}

static int ORBISPAD_JoystickOpen(SDL_Joystick *joystick, int device_index)
{
    if (device_index < 0 || device_index >= orbis_num_pads) {
        return SDL_SetError("No joystick available with that index");
    }

    joystick->instance_id = orbis_pads[device_index].instance_id;
    joystick->hwdata = (struct joystick_hwdata *)&orbis_pads[device_index];
    joystick->nbuttons = ORBIS_PAD_NBUTTONS;
    joystick->naxes = ORBIS_PAD_NAXES;
    joystick->nhats = 1;
    joystick->epowerlevel = SDL_JOYSTICK_POWER_WIRED;

    ORBISPAD_Log("open: device %d user=0x%08x handle=0x%08x instance=%d",
                 device_index, (unsigned)orbis_pads[device_index].user_id,
                 (unsigned)orbis_pads[device_index].handle, (int)joystick->instance_id);
    return 0;
}

static int ORBISPAD_JoystickRumble(SDL_Joystick *joystick, Uint16 low_frequency_rumble, Uint16 high_frequency_rumble)
{
    OrbisPadSlot *slot = ORBISPAD_SlotFor(joystick);
    int32_t rc;

    if (!slot) {
        return SDL_SetError("Joystick not open");
    }

    /* 8-bit motors; the large one is the low-frequency motor. */
    slot->vibe.lgMotor = (uint8_t)(low_frequency_rumble >> 8);
    slot->vibe.smMotor = (uint8_t)(high_frequency_rumble >> 8);
    rc = scePadSetVibration(slot->handle, &slot->vibe);
    if (rc < 0) {
        return SDL_SetError("scePadSetVibration failed: 0x%08x", (unsigned)rc);
    }
    return 0;
}

static int ORBISPAD_JoystickRumbleTriggers(SDL_Joystick *joystick, Uint16 left_rumble, Uint16 right_rumble)
{
    return SDL_Unsupported();
}

static Uint32 ORBISPAD_JoystickGetCapabilities(SDL_Joystick *joystick)
{
    return SDL_JOYCAP_LED | SDL_JOYCAP_RUMBLE;
}

static int ORBISPAD_JoystickSetLED(SDL_Joystick *joystick, Uint8 red, Uint8 green, Uint8 blue)
{
    OrbisPadSlot *slot = ORBISPAD_SlotFor(joystick);
    OrbisPadColor color;
    int32_t rc;

    if (!slot) {
        return SDL_SetError("Joystick not open");
    }

    color.r = red;
    color.g = green;
    color.b = blue;
    color.a = 0xFF;
    rc = scePadSetLightBar(slot->handle, &color);
    if (rc < 0) {
        return SDL_SetError("scePadSetLightBar failed: 0x%08x", (unsigned)rc);
    }
    return 0;
}

static int ORBISPAD_JoystickSendEffect(SDL_Joystick *joystick, const void *data, int size)
{
    return SDL_Unsupported();
}

static int ORBISPAD_JoystickSetSensorsEnabled(SDL_Joystick *joystick, SDL_bool enabled)
{
    return SDL_Unsupported();
}

/* 0..255 with 128 at rest -> -32768..32767 (0 -> -32768, 128 -> 128, 255 -> 32767). */
static SDL_INLINE Sint16 ORBISPAD_StickToAxis(uint8_t v)
{
    return (Sint16)((int)v * 257 - 32768);
}

static void ORBISPAD_ReportReleased(SDL_Joystick *joystick)
{
    int i;

    for (i = 0; i < ORBIS_PAD_NBUTTONS; i++) {
        SDL_PrivateJoystickButton(joystick, (Uint8)i, SDL_RELEASED);
    }
    for (i = 0; i < 4; i++) {
        SDL_PrivateJoystickAxis(joystick, (Uint8)i, 0);
    }
    SDL_PrivateJoystickAxis(joystick, 4, SDL_JOYSTICK_AXIS_MIN);
    SDL_PrivateJoystickAxis(joystick, 5, SDL_JOYSTICK_AXIS_MIN);
    SDL_PrivateJoystickHat(joystick, 0, SDL_HAT_CENTERED);
}

static void ORBISPAD_JoystickUpdate(SDL_Joystick *joystick)
{
    OrbisPadSlot *slot = ORBISPAD_SlotFor(joystick);
    OrbisPadData data;
    uint32_t buttons;
    Uint8 hat = SDL_HAT_CENTERED;
    int32_t rc;
    int i;

    if (!slot) {
        return;
    }

    SDL_zero(data);
    rc = scePadReadState(slot->handle, &data);
    if (rc < 0) {
        /* A failed read says nothing about the buttons; do not latch them. */
        if (!slot->read_failing) {
            ORBISPAD_Log("update: scePadReadState(handle=0x%08x) failed rc=0x%08x", (unsigned)slot->handle, (unsigned)rc);
            slot->read_failing = SDL_TRUE;
        }
        ORBISPAD_ReportReleased(joystick);
        return;
    }
    if (slot->read_failing) {
        ORBISPAD_Log("update: scePadReadState(handle=0x%08x) recovered", (unsigned)slot->handle);
        slot->read_failing = SDL_FALSE;
    }

    if (!data.connected) {
        if (slot->connected) {
            ORBISPAD_Log("update: pad user=0x%08x disconnected (slot kept, same handle)", (unsigned)slot->user_id);
            slot->connected = SDL_FALSE;
        }
        ORBISPAD_ReportReleased(joystick);
        return;
    }
    if (!slot->connected) {
        ORBISPAD_Log("update: pad user=0x%08x connected again", (unsigned)slot->user_id);
        slot->connected = SDL_TRUE;
    }

    buttons = data.buttons;
    for (i = 0; i < ORBIS_PAD_NBUTTONS; i++) {
        SDL_PrivateJoystickButton(joystick, (Uint8)i, (buttons & orbis_button_map[i]) ? SDL_PRESSED : SDL_RELEASED);
    }

    if (buttons & ORBIS_PAD_BUTTON_UP) {
        hat |= SDL_HAT_UP;
    }
    if (buttons & ORBIS_PAD_BUTTON_RIGHT) {
        hat |= SDL_HAT_RIGHT;
    }
    if (buttons & ORBIS_PAD_BUTTON_DOWN) {
        hat |= SDL_HAT_DOWN;
    }
    if (buttons & ORBIS_PAD_BUTTON_LEFT) {
        hat |= SDL_HAT_LEFT;
    }
    SDL_PrivateJoystickHat(joystick, 0, hat);

    SDL_PrivateJoystickAxis(joystick, 0, ORBISPAD_StickToAxis(data.leftStick.x));
    SDL_PrivateJoystickAxis(joystick, 1, ORBISPAD_StickToAxis(data.leftStick.y));
    SDL_PrivateJoystickAxis(joystick, 2, ORBISPAD_StickToAxis(data.rightStick.x));
    SDL_PrivateJoystickAxis(joystick, 3, ORBISPAD_StickToAxis(data.rightStick.y));
    SDL_PrivateJoystickAxis(joystick, 4, ORBISPAD_StickToAxis(data.analogButtons.l2));
    SDL_PrivateJoystickAxis(joystick, 5, ORBISPAD_StickToAxis(data.analogButtons.r2));
}

static void ORBISPAD_JoystickClose(SDL_Joystick *joystick)
{
    OrbisPadSlot *slot = ORBISPAD_SlotFor(joystick);

    /* The handle stays open for the life of the process; just stop any rumble. */
    if (slot && (slot->vibe.lgMotor || slot->vibe.smMotor)) {
        SDL_zero(slot->vibe);
        scePadSetVibration(slot->handle, &slot->vibe);
    }
    joystick->hwdata = NULL;
}

static void ORBISPAD_JoystickQuit(void)
{
    /* Handles are deliberately kept (see the header comment). Instance ids are
       re-issued by the next ORBISPAD_JoystickInit. */
}

static SDL_bool ORBISPAD_JoystickGetGamepadMapping(int device_index, SDL_GamepadMapping *out)
{
    /* Fallback only: SDL_gamecontrollerdb.h carries the same layout for this GUID. */
    out->a.kind = EMappingKind_Button;
    out->a.target = 0;
    out->b.kind = EMappingKind_Button;
    out->b.target = 1;
    out->x.kind = EMappingKind_Button;
    out->x.target = 2;
    out->y.kind = EMappingKind_Button;
    out->y.target = 3;
    out->leftshoulder.kind = EMappingKind_Button;
    out->leftshoulder.target = 4;
    out->rightshoulder.kind = EMappingKind_Button;
    out->rightshoulder.target = 5;
    out->leftstick.kind = EMappingKind_Button;
    out->leftstick.target = 6;
    out->rightstick.kind = EMappingKind_Button;
    out->rightstick.target = 7;
    out->start.kind = EMappingKind_Button;
    out->start.target = 8;
    out->back.kind = EMappingKind_Button;
    out->back.target = 9;
    out->touchpad.kind = EMappingKind_Button;
    out->touchpad.target = 9;

    out->dpup.kind = EMappingKind_Hat;
    out->dpup.target = 0x01;
    out->dpright.kind = EMappingKind_Hat;
    out->dpright.target = 0x02;
    out->dpdown.kind = EMappingKind_Hat;
    out->dpdown.target = 0x04;
    out->dpleft.kind = EMappingKind_Hat;
    out->dpleft.target = 0x08;

    out->leftx.kind = EMappingKind_Axis;
    out->leftx.target = 0;
    out->lefty.kind = EMappingKind_Axis;
    out->lefty.target = 1;
    out->rightx.kind = EMappingKind_Axis;
    out->rightx.target = 2;
    out->righty.kind = EMappingKind_Axis;
    out->righty.target = 3;
    out->lefttrigger.kind = EMappingKind_Axis;
    out->lefttrigger.target = 4;
    out->righttrigger.kind = EMappingKind_Axis;
    out->righttrigger.target = 5;
    return SDL_TRUE;
}

SDL_JoystickDriver SDL_ORBIS_JoystickDriver = {
    ORBISPAD_JoystickInit,
    ORBISPAD_JoystickGetCount,
    ORBISPAD_JoystickDetect,
    ORBISPAD_JoystickGetDeviceName,
    ORBISPAD_JoystickGetDevicePath,
    ORBISPAD_JoystickGetDeviceSteamVirtualGamepadSlot,
    ORBISPAD_JoystickGetDevicePlayerIndex,
    ORBISPAD_JoystickSetDevicePlayerIndex,
    ORBISPAD_JoystickGetDeviceGUID,
    ORBISPAD_JoystickGetDeviceInstanceID,

    ORBISPAD_JoystickOpen,

    ORBISPAD_JoystickRumble,
    ORBISPAD_JoystickRumbleTriggers,

    ORBISPAD_JoystickGetCapabilities,
    ORBISPAD_JoystickSetLED,
    ORBISPAD_JoystickSendEffect,
    ORBISPAD_JoystickSetSensorsEnabled,

    ORBISPAD_JoystickUpdate,
    ORBISPAD_JoystickClose,
    ORBISPAD_JoystickQuit,
    ORBISPAD_JoystickGetGamepadMapping,
};

#endif /* SDL_JOYSTICK_ORBIS */

/* vi: set ts=4 sw=4 expandtab: */
