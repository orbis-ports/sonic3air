/* PS4 (OpenOrbis) shim: the target triple is FreeBSD, so SDL_endian.h asks for <sys/endian.h>,
 * but the SDK's libc headers are musl's, which have <endian.h> instead. */
#pragma once
#include <endian.h>
