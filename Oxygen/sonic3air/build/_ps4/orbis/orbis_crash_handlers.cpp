/* Sonic 3 A.I.R. PS4 port - installs orbis-compat's crash handlers.
 *
 * Adapted from RetroArch ps4/orbis_crash_handlers.cpp (MIT, same maintainer). The RetroArch version
 * had to point orbis_log / orbis_log_fatal at its own ps4_log channel first; here ps4_app_init()
 * (orbis-compat optional/ps4_app.cpp) already registers both sinks - UDP netlog for ordinary lines,
 * klog-first for fatal ones - so the only thing left to do is to install the handlers.
 *
 * ⚠ CALL IT AFTER ps4_app_init(). installCrashHandlers() reports what it managed through orbis_log,
 * and before ps4_app_init() that is a null sink: the handlers would be installed and nothing would
 * say so, and a later "fatal: signal" line would go nowhere either.
 *
 * ⚠ AND IT DOES NOT REPLACE orbis_abort_report.c, which catches a different class entirely:
 * abort_message() and __assert_fail() are CALLS a dying library makes on purpose, not signals.
 *
 * SPDX-License-Identifier: MIT
 */
#include <orbis_boot.h>

extern "C" void orbis_install_crash_handlers(void)
{
   orbis::installCrashHandlers();
}
