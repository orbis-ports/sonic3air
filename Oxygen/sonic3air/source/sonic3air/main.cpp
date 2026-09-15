/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/EngineDelegate.h"
#include "sonic3air/GameArgumentsReader.h"
#include "sonic3air/helper/PackageBuilder.h"
#include "sonic3air/platform/PlatformSpecifics.h"

#include "oxygen/platform/CommandForwarder.h"
#include "oxygen/platform/PlatformFunctions.h"

#ifdef RMX_USE_SDL3
	#include <SDL3/SDL_main.h>
#endif

#if defined(PLATFORM_PS4)
	#include <ps4_app.h>			// orbis-compat optional/: UDP netlog + klog, termination policy
	#include <orbis_paths.h>		// orbis_set_anchor_root
	#include <orbis/SystemService.h>
	#include "rmxbase/tools/PS4Stage.h"
	#include <cstdio>
	#include <cstdlib>
	#include <cstring>
	extern "C" void orbis_install_crash_handlers(void);		// Oxygen/sonic3air/build/_ps4/orbis/orbis_crash_handlers.cpp
#endif


// [Added for Switch platform] HJW: I know it's sloppy to put this here... it'll get moved afterwards
// Building with my env (msys2,gcc) requires this stub for some reason
#ifndef pathconf
long pathconf(const char* path, int name)
{
	errno = ENOSYS;
	return -1;
}
#endif

#if defined(PLATFORM_WINDOWS) & !defined(__GNUC__)
extern "C"
{
	// Tell graphics drivers to prefer the dedicated GPU, if there's integrated graphics as well
	_declspec(dllexport) uint32 NvOptimusEnablement = 1;
	_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

#if defined(PLATFORM_VITA)
extern "C"
{
	// Any value higher than 324 MB will make the game either boot without sound or just crash the PSVITA due to lack of physical RAM
	int _newlib_heap_size_user = 324 * 1024 * 1024;
	unsigned int sceUserMainThreadStackSize = 4 * 1024 * 1024;
}
#endif


#if defined(PLATFORM_PS4)
namespace
{
	// Never return from main() on PS4: that tears the process down outside the system's expected path and
	// shows error CE-34878-0. sceSystemServiceLoadExec("exit") hands the process back to the system
	// (measured on hardware in the RetroArch port: it does not return). If it ever does, idle instead.
	[[noreturn]] void ps4Exit(const char* reason)
	{
		PS4_STAGE("exit: %s", reason);
		const int32_t rc = sceSystemServiceLoadExec("exit", nullptr);
		ps4_log("sceSystemServiceLoadExec(\"exit\") returned 0x%08x - idling instead", (unsigned)rc);
		ps4_idle_forever(reason);	// Returns only with "autoexit=1" in /app0/ps4-run.cfg (emulator runs)
		_Exit(0);
	}

	// Environment knobs for Mesa and this port, before anything touches the GPU stack.
	//  -> Mesa's log goes to stderr, which on this console is the kernel debug channel (klog): 8-15 ms per line,
	//     blocking the calling thread. Its default level is "info", and during gameplay zink's busy-buffer polls produced
	//     ~150 "syncobj wait timed out" warnings per second (plus periodic info lines from the WSI and the submit path).
	//     So the default here is "error". Mesa reads MESA_LOG_LEVEL once, at its first log call (video init).
	//  -> "/data/sonic3air-env.txt" (optional): KEY=VALUE per line, '#' comments, applied with overwrite. This is how
	//     diagnostics or strategies are toggled on the console without a rebuild (see sonic3air-env.example.txt).
	void ps4ApplyEnvironment()
	{
		static const char* ENV_FILE = "/data/sonic3air-env.txt";
		setenv("MESA_LOG_LEVEL", "error", 0);

		FILE* file = fopen(ENV_FILE, "r");
		if (nullptr == file)
		{
			ps4_log("env: no %s - defaults only", ENV_FILE);
		}
		else
		{
			char line[512];
			int applied = 0;
			while (nullptr != fgets(line, sizeof(line), file))
			{
				char* newline = strpbrk(line, "\r\n");
				if (nullptr != newline)
					*newline = 0;

				char* key = line;
				while (*key == ' ' || *key == '\t')
					++key;
				if (*key == 0 || *key == '#')
					continue;

				char* equals = strchr(key, '=');
				if (nullptr == equals)
				{
					ps4_log("env: ignoring '%s' - no '='", key);
					continue;
				}

				// Trim both sides of the '=' and the end of the value
				char* keyEnd = equals;
				while (keyEnd > key && (keyEnd[-1] == ' ' || keyEnd[-1] == '\t'))
					--keyEnd;
				*keyEnd = 0;
				char* value = equals + 1;
				while (*value == ' ' || *value == '\t')
					++value;
				char* valueEnd = value + strlen(value);
				while (valueEnd > value && (valueEnd[-1] == ' ' || valueEnd[-1] == '\t'))
					--valueEnd;
				*valueEnd = 0;
				if (*key == 0)
					continue;

				setenv(key, value, 1);
				ps4_log("env: %s=%s (from %s)", key, value, ENV_FILE);
				++applied;
			}
			fclose(file);
			ps4_log("env: %d knob(s) applied from %s", applied, ENV_FILE);
		}

		const char* level = getenv("MESA_LOG_LEVEL");
		ps4_log("env: effective MESA_LOG_LEVEL=%s", (nullptr != level) ? level : "(unset)");
	}
}
#endif


int main(int argc, char** argv)
{
#if defined(PLATFORM_PS4)
	// Before anything else: the log channel (klog "alive" line + UDP netlog to the dev host)
	ps4_app_init("sonic3air", PS4_APP_STAMP);
	PS4_STAGE("app init (argc=%d)", argc);

	// Before SDL/EGL/Mesa initialization
	ps4ApplyEnvironment();

	// No working directory on this platform: relative paths ("data/gamedata.bin", "config.json", ...)
	// are read from the read-only package root. Must happen before the first relative path is opened.
	orbis_set_anchor_root("/app0/");

	// SIGSEGV/SIGBUS/SIGILL/SIGFPE/SIGABRT + std::terminate reports, through the sinks ps4_app_init registered
	orbis_install_crash_handlers();
#endif

	EngineMain::earlySetup();
	PlatformSpecifics::platformStartup();
	CommandForwarder::setApplicationName("S3AIR");

	GameArgumentsReader arguments;

#if defined(PLATFORM_VITA) || defined(PLATFORM_PS4)
	argc = 0;
#else
	// Read command line arguments
	arguments.read(argc, argv);

	// For certain arguments, just try to forward them to an already running instance of S3AIR
	if (!arguments.mForwardedCommand.empty())
	{
		if (CommandForwarder::trySendCommand("ForwardedCommand:" + arguments.mForwardedCommand))
			return 0;
	}
	if (!arguments.mUrl.empty())
	{
		if (CommandForwarder::trySendCommand("ForwardedCommand:Url:" + arguments.mUrl))
			return 0;
	}
	if (arguments.mStop)
		return 0;

	// Make sure we're in the correct working directory
	PlatformFunctions::changeWorkingDirectory(arguments.mExecutableCallPath);
#endif

#if defined(PLATFORM_WINDOWS)
	// Check if the user has an old version of "audioremaster.bin", and remove it if that's the case
	//  -> As the newer installations don't include that file, it is most likely an out-dated one, and could cause problems (at least an assert) down the line
	if (FTX::FileSystem->exists(L"data/audioremaster.bin"))
		FTX::FileSystem->removeFile(L"data/audioremaster.bin");
#endif

#if !defined(PLATFORM_ANDROID) && !defined(PLATFORM_VITA) && !defined(PLATFORM_PS4)
	if (arguments.mPack)
	{
		PackageBuilder::performPacking();
		if (!arguments.mNativize && !arguments.mDumpCppDefinitions)		// In case multiple arguments got combined, the others would get ignored without this check
			return 0;
	}
#endif

	// Randomization is quite important for server communication
	randomize();

	try
	{
		// Create engine delegate and engine main instance
		EngineDelegate myDelegate;
		EngineMain myMain(myDelegate, arguments);

		// Evaluate some more arguments
		Configuration& config = Configuration::instance();
		if (arguments.mNativize)
		{
			config.mRunScriptNativization = 1;
			config.mScriptNativizationOutput = L"source/sonic3air/_nativized/NativizedCode.inc";
			config.mExitAfterScriptLoading = true;
		}
		if (arguments.mDumpCppDefinitions)
		{
			config.mDumpCppDefinitionsOutput = L"scripts/_reference/cpp_core_functions.lemon";
			config.mExitAfterScriptLoading = true;
		}
		if (arguments.mCompileScripts)
		{
			// Compile the scripts to "saves/scripts.bin" (to be shipped as "data/scripts.bin") without needing a ROM
			config.mCompileScriptsOnly = true;
			config.mExitAfterScriptLoading = true;
		}

		// Now run the game
		myMain.execute();

		if (arguments.mCompileScripts)
		{
			// Exit code for build scripts: the output only exists if compilation succeeded (it gets removed before compiling)
			const bool success = !config.mCompiledScriptSavePath.empty() && FTX::FileSystem->exists(config.mCompiledScriptSavePath);
			RMX_LOG_INFO("Script compilation " << (success ? "succeeded" : "FAILED"));
			return success ? 0 : 1;
		}
	}
	catch (const std::exception& e)
	{
	#if defined(PLATFORM_PS4)
		PS4_STAGE("unhandled exception in main loop: %s", e.what());
	#endif
		RMX_ERROR("Caught unhandled exception in main loop: " << e.what(), );
	}

#if defined(PLATFORM_PS4)
	ps4Exit("engine finished");
#endif
	return 0;
}
