/* The first line this process writes, before any C++ static initializer of the game runs.
 *
 * ⚠ WHY: the wchar_t crash (orbis_wchar32.c) happened in a static initializer - before main(), before
 * ps4_app_init() and the UDP netlog - and klog showed only a fatal signal. With this line in klog a
 * pre-main crash is distinguishable from a process that never started at all.
 *
 * ⚠ WHY NOT constructor(101): clang emits prioritized constructors into .init_array.<N>, and
 * orbis-tls.ld only names the bare .init_array. lld would place the .101 section as an orphan AFTER
 * every unprioritized constructor - last, not first. So this is a plain constructor, and
 * CMakeLists.txt lists this file FIRST in the executable's sources: .init_array runs in link order,
 * which puts this ahead of every game/engine/SDL initializer (those come from later objects and from
 * archives). Only liborbis-compat.a and RADV (--whole-archive, earlier on the line) run before it.
 *
 * klog (sceKernelDebugOutText) because it is synchronous and needs nothing initialised; one line, so
 * it costs one ~10 ms kernel call per launch.
 *
 * SPDX-License-Identifier: MIT
 */
#include <orbis/libkernel.h>

__attribute__((constructor))
static void orbis_preinit_breadcrumb(void)
{
	sceKernelDebugOutText(0, "[sonic3air] S3AIR_PREINIT start (first game constructor, before C++ static init)\n");
}
