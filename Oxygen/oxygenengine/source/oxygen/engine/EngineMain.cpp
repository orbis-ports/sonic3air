/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/engine/EngineMain.h"
#include "oxygen/engine/EngineSystems.h"
#include "oxygen/application/Application.h"
#include "oxygen/application/ArgumentsReader.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/application/GameProfile.h"
#include "oxygen/application/audio/AudioOutBase.h"
#include "oxygen/drawing/opengl/OpenGLDrawer.h"
#include "oxygen/drawing/software/SoftwareDrawer.h"
#include "oxygen/file/PackedFileProvider.h"
#include "oxygen/helper/FileHelper.h"
#include "oxygen/helper/JsonHelper.h"
#include "oxygen/menu/imgui/ImGuiIntegration.h"
#include "oxygen/platform/CrashHandler.h"
#include "oxygen/platform/PlatformFunctions.h"
#include "oxygen/rendering/RenderResources.h"
#include "oxygen/simulation/Simulation.h"

#include "rmxbase/tools/PS4Stage.h"


#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX)
	#define LOAD_APP_ICON_PNG
#endif


void EngineMain::earlySetup()
{
	// This function contains stuff you would usually do right at the start of the "main" function

	// Setup crash handling
	CrashHandler::initializeCrashHandler();

#ifdef PLATFORM_WINDOWS
	// This fixes some audio issues with SDL 2.0.9 that some people faced
	// (possibly introduced earlier, only 2.0.4 is known to have worked)
	#ifdef RMX_USE_SDL3
		SDL_setenv_unsafe("SDL_AUDIODRIVER", "directsound", true);
	#else
		SDL_setenv("SDL_AUDIODRIVER", "directsound", true);
	#endif
#endif

	SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "0");

#if defined(PLATFORM_PS4)
	// SDL's dummy drivers are "demand only": without the orbis drivers (PS4_SDL_ORBIS_VIDEO / PS4_SDL_ORBIS_AUDIO
	// build options) they have to be requested by name, or SDL video / audio initialization fails outright
	#if !defined(SDL_VIDEO_DRIVER_ORBIS)
		SDL_SetHint(SDL_HINT_VIDEODRIVER, "dummy");
	#endif
	#if !defined(SDL_AUDIO_DRIVER_ORBIS)
		SDL_SetHint(SDL_HINT_AUDIODRIVER, "dummy");
	#endif
#endif

	INIT_RMX;
	INIT_RMXEXT_OGGVORBIS;
}

EngineMain::EngineMain(EngineDelegateInterface& delegate_, ArgumentsReader& arguments) :
	mDelegate(delegate_),
	mArguments(arguments),
	mSystems(*new oxygen::EngineSystems())
{
}

EngineMain::~EngineMain()
{
	delete &mSystems;
}

void EngineMain::execute()
{
	// Startup the Oxygen Engine part that is independent from the application / project
	if (startupEngine())
	{
		// Enter the application run loop
		run();
	}

	// Done, now shut everything down
	shutdown();
}

void EngineMain::onActiveModsChanged()
{
	// Update sprites
	RenderResources::instance().loadSprites(true);

	// Update the resource cache -> palettes, raw data
	ResourcesCache::instance().loadAllResources();

	// Update fonts
	mSystems.mFontCollection.collectFromMods();

	// Update video
	mSystems.mVideoOut.handleActiveModsChanged();

	// Update audio
	mAudioOut->handleActiveModsChanged();

	// Update input
	mSystems.mInputManager.handleActiveModsChanged();

	// Scripts need to be reloaded
	Application::instance().getSimulation().reloadScriptsAfterModsChange();

	// Inform the application
	Application::instance().onActiveModsChanged();

	// Inform the delegate as well
	mDelegate.onActiveModsChanged();
}

bool EngineMain::reloadFilePackage(std::wstring_view packageName, bool forceReload)
{
	GameProfile& gameProfile = GameProfile::instance();
	for (size_t index = 0; index < gameProfile.mDataPackages.size(); ++index)
	{
		const GameProfile::DataPackage& dataPackage = gameProfile.mDataPackages[index];
		if (dataPackage.mFilename == packageName)
		{
			return loadFilePackageByIndex(index, forceReload);
		}
	}
	return false;
}

uint32 EngineMain::getPlatformFlags() const
{
	if (Configuration::instance().mPlatformFlags != -1)
	{
		return Configuration::instance().mPlatformFlags;
	}
	else
	{
		uint32 flags = 0;
	#if defined(PLATFORM_IS_DESKTOP)
		flags |= 0x0001;
	#elif defined(PLATFORM_IS_MOBILE)
		flags |= 0x0002;
	#endif
		return flags;
	}
}

void EngineMain::switchToRenderMethod(Configuration::RenderMethod newRenderMethod)
{
	Configuration& config = Configuration::instance();
	const bool wasUsingOpenGL = (config.mRenderMethod == Configuration::RenderMethod::OPENGL_FULL || config.mRenderMethod == Configuration::RenderMethod::OPENGL_SOFT);
#if defined(PLATFORM_PS4)
	const Configuration::RenderMethod previousRenderMethod = config.mRenderMethod;
#endif
	config.mRenderMethod = newRenderMethod;

	bool nowUsingOpenGL = (config.mRenderMethod == Configuration::RenderMethod::OPENGL_FULL || config.mRenderMethod == Configuration::RenderMethod::OPENGL_SOFT);
#if defined(PLATFORM_PS4)
	PS4_STAGE("render method switch %d -> %d (%s)", (int)previousRenderMethod, (int)newRenderMethod, (nowUsingOpenGL != wasUsingOpenGL) ? "window gets recreated" : "same window, renderer only");
#endif
	if (nowUsingOpenGL != wasUsingOpenGL)
	{
		// Need to recreate the window
		destroyWindow();
	#if defined(PLATFORM_PS4)
		// Do not continue without a window: go back to the previous render method if the new one cannot get one
		if (createWindow())
		{
			PS4_STAGE("video re-init done (render method %d)", (int)config.mRenderMethod);
		}
		else
		{
			PS4_STAGE("video re-init FAILED for render method %d (%s) - back to render method %d", (int)newRenderMethod, SDL_GetError(), (int)previousRenderMethod);
			config.mRenderMethod = previousRenderMethod;
			if (createWindow())
				PS4_STAGE("video re-init done (render method %d, previous one restored)", (int)config.mRenderMethod);
			else
				PS4_STAGE("video re-init FAILED again for render method %d (%s)", (int)previousRenderMethod, SDL_GetError());
		}
	#else
		createWindow();
	#endif

		// Check OpenGL in the config again, it could have changed - namely if OpenGL initialization failed
		nowUsingOpenGL = (config.mRenderMethod == Configuration::RenderMethod::OPENGL_FULL || config.mRenderMethod == Configuration::RenderMethod::OPENGL_SOFT);

		if (ImGuiIntegration::hasInstance())
			ImGuiIntegration::instance().onWindowRecreated(nowUsingOpenGL);
	}

	if (nowUsingOpenGL)
	{
		config.mAutoDetectRenderMethod = false;
	}

	// Switch the renderer
	VideoOut::instance().createRenderer(true);
}

void EngineMain::setVSyncMode(Configuration::FrameSyncType frameSyncMode)
{
	Configuration& config = Configuration::instance();
	if ((config.mRenderMethod == Configuration::RenderMethod::OPENGL_FULL) || (config.mRenderMethod == Configuration::RenderMethod::OPENGL_SOFT))
	{
		if (frameSyncMode >= Configuration::FrameSyncType::VSYNC_ON)
		{
			SDL_GL_SetSwapInterval(1);
		}
		else
		{
			SDL_GL_SetSwapInterval(0);
		}
	}
}

Vec2i EngineMain::getDisplaySize(int displayIndex) const
{
	SDL_Rect rect;
	if (SDL_GetDisplayBounds(displayIndex, &rect) == 0)
	{
		return Vec2i(rect.w, rect.h);
	}
	else
	{
	#ifdef RMX_USE_SDL3
		const SDL_DisplayMode* dm = SDL_GetDesktopDisplayMode(displayIndex);
		if (nullptr != dm)
		{
			return Vec2i(dm->w, dm->h);
		}
	#else
		SDL_DisplayMode dm;
		if (SDL_GetDesktopDisplayMode(displayIndex, &dm) == 0)
		{
			return Vec2i(dm.w, dm.h);
		}
	#endif
	}

	// Return some fallback size in case everything failed... how about Full HD?
	return Vec2i(1920, 1080);
}

bool EngineMain::startupEngine()
{
#if defined(PLATFORM_ANDROID)
	{
		// Create file provider for APK content access (and do it right here already)
		rmx::FileProviderSDL* provider = new rmx::FileProviderSDL();
		FTX::FileSystem->addManagedFileProvider(*provider);
		FTX::FileSystem->addMountPoint(*provider, L"", L"", 1);
	}
#endif

	PlatformFunctions::onEngineStartup();

	if (!mDelegate.onEnginePreStartup())
		return false;

	const EngineDelegateInterface::AppMetaData& appMetaData = mDelegate.getAppMetaData();
	Configuration& config = Configuration::instance();

#ifndef RMX_USE_SDL3
	// Don't use the accelerometer as a joystick on mobile devices, that's just confusing
	SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
#endif

	// Disable the screen saver and hopefully also system sleep (which makes especially sense when playing with a game controller)
	//  -> It should be disabled by default according to the SDL2 docs, but that does not seem to be always the case
	SDL_DisableScreenSaver();

	// Determine various directory and file paths in config
	initDirectories();

	// Startup logging
	{
		oxygen::Logging::startup(config.mAppDataPath + L"logfile.txt");
		RMX_LOG_INFO("--- STARTUP ---");
		RMX_LOG_INFO("Logging started");
		RMX_LOG_INFO("Application version: " << appMetaData.mBuildVersionString);
		RMX_LOG_INFO("Executable path:     " << WString(mArguments.mExecutableCallPath).toStdString());
		RMX_LOG_INFO("App data path:       " << WString(config.mAppDataPath).toStdString());
	}

	PS4_STAGE("logging started, app data path '%s'", WString(config.mAppDataPath).toStdString().c_str());

	// Load configuration and settings
	if (!initConfigAndSettings())
	{
		PS4_STAGE("config load FAILED");
		return false;
	}
	PS4_STAGE("config loaded (render method %d, window mode %d, %dx%d)", (int)config.mRenderMethod, (int)config.mWindowMode, config.mWindowSize.x, config.mWindowSize.y);

	// Setup file system
	RMX_LOG_INFO("File system setup");
	if (!initFileSystem())
	{
		PS4_STAGE("file system / data packages FAILED");
		return false;
	}

	// System
	RMX_LOG_INFO("System initialization...");
	if (!FTX::System->initialize())
	{
		RMX_ERROR("System initialization failed", );
		return false;
	}

	// Video
	RMX_LOG_INFO("Loading upscaler definitions...");
	mSystems.mUpscalerCollection.loadUpscalers();

	RMX_LOG_INFO("Video initialization...");
	PS4_STAGE("video init...");
	if (!createWindow())
	{
		PS4_STAGE("video init FAILED: %s", SDL_GetError());
		RMX_ERROR("Unable to create window" << (config.mFailSafeMode ? " in fail-safe mode" : "") << " with error: " << SDL_GetError(), );
		return false;
	}

	PS4_STAGE("video init done (render method %d, SDL video driver '%s')", (int)config.mRenderMethod, SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "(none)");

	RMX_LOG_INFO("Startup of VideoOut...");
	mSystems.mVideoOut.startup();

	// Input manager startup after config is loaded
	RMX_LOG_INFO("Input initialization...");
	InputManager::instance().startup();

	// Audio
	RMX_LOG_INFO("Audio initialization...");
	FTX::Audio->initialize(config.mAudio.mSampleRate, 2, 1024);

	RMX_LOG_INFO("Startup of AudioOut");
	mAudioOut = &EngineMain::getDelegate().createAudioOut();
	mAudioOut->startup();
	PS4_STAGE("audio init done (%d Hz, SDL audio driver '%s')", config.mAudio.mSampleRate, SDL_GetCurrentAudioDriver() ? SDL_GetCurrentAudioDriver() : "(none)");

	// Networking
	RMX_LOG_INFO("Networking initialization...");
	const bool useIPv6 = false;
	mSystems.mEngineServerClient.setupClient(useIPv6);	// Does nothing on PS4, where online features are disabled

	// Command forwarder
	mSystems.mCommandForwarder.startup();

	// Test extension
	mSystems.mTestExtension.initialize();

	// Done
	RMX_LOG_INFO("Engine startup successful");
	PS4_STAGE("engine startup successful");
	return true;
}

void EngineMain::run()
{
	// Run RMX application
	RMX_LOG_INFO("");
	RMX_LOG_INFO("--- MAIN LOOP ---");
	RMX_LOG_INFO("Starting main application loop");

	Application application;
	PS4_STAGE("entering main loop");
	FTX::System->run(application);
	PS4_STAGE("main loop left");
}

void EngineMain::shutdown()
{
	mSystems.mCommandForwarder.shutdown();

	destroyWindow();

	// Shutdown subsystems
	mSystems.mVideoOut.shutdown();
	if (nullptr != mAudioOut)
	{
		mAudioOut->shutdown();
		SAFE_DELETE(mAudioOut);
	}

	// Shutdown drawer
	mDrawer.shutdown();

	// Cleanup system
	RMX_LOG_INFO("System shutdown");
	FTX::Audio->exit();
	FTX::JobManager->~JobManager();
	FTX::System->exit();

	mSystems.mModManager.copyModSettingsToConfig();
	Configuration::instance().saveSettings();
	oxygen::Logging::shutdown();
}

void EngineMain::initDirectories()
{
	const EngineDelegateInterface::AppMetaData& appMetaData = mDelegate.getAppMetaData();
	Configuration& config = Configuration::instance();

#if !defined(PLATFORM_ANDROID) && !defined(PLATFORM_VITA)
	config.mExePath = mArguments.mExecutableCallPath;
#endif

	// Get app data path
	{
	#if defined(PLATFORM_ANDROID)
		// Android
		// TODO: Use internal storage path as a fallback?
		WString storagePath = String(SDL_AndroidGetExternalStoragePath()).toWString();
		config.mAppDataPath = *(storagePath + L'/');
	#elif defined(PLATFORM_VITA)
		// Vita
		config.mAppDataPath = L"ux0:data/sonic3air/savedata/";
	#elif defined(PLATFORM_PS4)
		// PS4: the package root /app0/ is read-only, /data/ is the writable area for homebrew
		config.mAppDataPath = L"/data/sonic3air/savedata/";
		FTX::FileSystem->createDirectory(config.mAppDataPath);
	#elif !defined(PLATFORM_IOS)
		// Choose app data path
		{
			const std::wstring appDataPath = PlatformFunctions::getAppDataPath();
			const bool useLocalSaveDataDirectory = (FTX::FileSystem->exists(L"savedata") || appMetaData.mAppDataFolder.empty() || appDataPath.empty());
			if (!useLocalSaveDataDirectory)
			{
				// This is the default case: Use the app data path
				config.mAppDataPath = appDataPath + L'/' + appMetaData.mAppDataFolder + L'/';
			}
			else
			{
				// Special case & fallback: Use local "savedata" path instead
				std::wstring currentDirectory = rmx::FileSystem::getCurrentDirectory();
				rmx::FileSystem::normalizePath(currentDirectory, true);
				config.mAppDataPath = currentDirectory + L"savedata/";
			}
		}
	#endif

		// In any case: Check for redirect there
		for (int iteration = 0; iteration < 3; ++iteration)
		{
			Json::Value redirectRoot = JsonHelper::loadFile(config.mAppDataPath + L"redirect.json");
			if (redirectRoot.isNull())
				break;

			JsonHelper rootHelper(redirectRoot);
			std::wstring redirectedPath;
			if (!rootHelper.tryReadString("Redirect", redirectedPath))
				break;

			rmx::FileSystem::normalizePath(redirectedPath, true);
			if (!FTX::FileSystem->exists(redirectedPath))
				break;

			config.mAppDataPath = redirectedPath;
		}
	}

	// Fill some paths with fallback values, even though we haven't loaded a game profile yet
	updateGameProfilePaths();
}

bool EngineMain::initConfigAndSettings()
{
	RMX_LOG_INFO("Initializing configuration");
	Configuration& config = Configuration::instance();
	config.initialization();

	RMX_LOG_INFO("Loading configuration");
	loadConfigJson();

	// Setup a custom game profile (like S3AIR does) or load the "oxygenproject.json"
	const bool hasCustomGameProfile = mDelegate.setupCustomGameProfile();
	if (!hasCustomGameProfile)
	{
		if (!mArguments.mProjectPath.empty() && FTX::FileSystem->exists(mArguments.mProjectPath + L"oxygenproject.json"))
		{
			// Overwrite project path from config
			config.mProjectPath = mArguments.mProjectPath;
		}

		RMX_LOG_INFO("Loading game profile");
		const bool loadedProject = mSystems.mGameProfile.loadOxygenProjectFromFile(config.mProjectPath + L"oxygenproject.json");
		RMX_CHECK(loadedProject, "Failed to load game profile from '" << *WString(config.mProjectPath).toString() << "oxygenproject.json'", );
	}

	updateGameProfilePaths();

	// Load settings
	RMX_LOG_INFO("Loading settings");
	const bool loadedSettings = config.loadSettings(config.mAppDataPath + L"settings.json", Configuration::SettingsType::STANDARD);
	config.loadSettings(config.mAppDataPath + L"settings_input.json", Configuration::SettingsType::INPUT);
	if (loadedSettings)
	{
	#if defined(PLATFORM_IS_DESKTOP)
		// Load config.json once again on top, so that config.json is preferred over settings.json
		if (!hasCustomGameProfile && !mArguments.mProjectPath.empty() && FTX::FileSystem->exists(mArguments.mProjectPath + L"oxygenproject.json"))
		{
			// Load project path's config.json, if there is one
			config.loadConfiguration(mArguments.mProjectPath + L"config.json");
		}
		else
		{
			loadConfigJson();
		}
	#endif

		// Remove old "settings_global.json", which was only used for legacy compatibility
		FTX::FileSystem->removeFile(config.mAppDataPath + L"settings_global.json");
	}
	else
	{
		// Save default settings once immediately
		config.saveSettings();
	}

	// Respect display index if set on the command line
	if (mArguments.mDisplayIndex >= 0)
	{
		config.mDisplayIndex = mArguments.mDisplayIndex;
	}

	// Enable dev mode if requested
	config.mDevMode.mEnabled = config.mDevMode.mEnableAtStartup;

	// Evaluate fail-safe mode
	if (config.mFailSafeMode)
	{
		RMX_LOG_INFO("Using fail-safe mode");
		config.mRenderMethod = Configuration::RenderMethod::SOFTWARE;	// Should already be set actually, but why not play it safe
	}
	else if (config.mRenderMethod == Configuration::RenderMethod::UNDEFINED)
	{
		config.mRenderMethod = Configuration::RenderMethod::OPENGL_FULL;
	}

	// Respect the platform's settings for supported render methods
	if (config.mRenderMethod > Configuration::getHighestSupportedRenderMethod())
		config.mRenderMethod = Configuration::getHighestSupportedRenderMethod();

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_IOS) || defined(PLATFORM_VITA) || defined(PLATFORM_PS4)
	// Use fullscreen, with no borders please
	//  -> Note that this doesn't work for the web version, if running in mobile browsers - we rely on a window with fixed size (see config.json) there
	config.mWindowMode = Configuration::WindowMode::FULLSCREEN_EXCLUSIVE;
#endif

#if defined(PLATFORM_PS4)
	// The PS4 video out is always 1920x1080, and sceAudioOut only takes 48 kHz
	config.mWindowSize.set(1920, 1080);
	config.mAudio.mSampleRate = 48000;
#endif

	RMX_LOG_INFO(((config.mRenderMethod == Configuration::RenderMethod::SOFTWARE) ? "Using pure software renderer" :
				  (config.mRenderMethod == Configuration::RenderMethod::OPENGL_SOFT) ? "Using opengl-soft renderer" : "Using opengl-full renderer"));
	return true;
}

void EngineMain::loadConfigJson()
{
	Configuration& config = Configuration::instance();
	if (FTX::FileSystem->exists(config.mAppDataPath + L"config.json"))
	{
		config.loadConfiguration(config.mAppDataPath + L"config.json");
	}
	else
	{
	#if (defined(PLATFORM_MAC) || defined(PLATFORM_IOS)) && defined(ENDUSER)
		config.loadConfiguration(config.mGameDataPath + L"/config.json");
	#else
		config.loadConfiguration(L"config.json");
	#endif
	}
}

void EngineMain::updateGameProfilePaths()
{
	Configuration& config = Configuration::instance();

	// Use an project-specific app data sub-folder path, unless the application defined its own app data folder (like the S3AIR executable does)
	if ((mDelegate.getAppMetaData().mAppDataFolder != L"OxygenEngine") || mSystems.mGameProfile.mIdentifier.empty())
	{
		config.mGameAppDataPath = config.mAppDataPath;
	}
	else
	{
		config.mGameAppDataPath = config.mAppDataPath + L"_" + String(mSystems.mGameProfile.mIdentifier).toStdWString() + L"/";
	}

	// Update dependent paths
	config.mSaveStatesDirLocal = config.mGameAppDataPath + L"savestates/";
	config.mPersistentDataBasePath = config.mGameAppDataPath + L"storage/";
}

bool EngineMain::initFileSystem()
{
	Configuration& config = Configuration::instance();

	if (mDelegate.isDedicatedApplication())
	{
		// Add Oxygen Engine data path if it exists in the expected place
		//  -> This is relevant when starting an external project app (like S3AIR) during development
		const std::wstring engineBasePath = L"../oxygenengine/";
		if (FTX::FileSystem->exists(engineBasePath))
		{
			rmx::RealFileProvider* provider = new rmx::RealFileProvider();
			FTX::FileSystem->addManagedFileProvider(*provider);
			FTX::FileSystem->addMountPoint(*provider, L"data/", engineBasePath + L"data/", 0x10);
		}
	}

	// In case the game data path isn't located in local "data" directory, add a real file system provider for it
	//  -> This is relevant for Oxygen Engine using an external game data path
	//  -> Also, the Mac build of S3AIR requires this logic, as game data is in a different subdirectory inside the app container than the binary
	//  -> In other cases (such as S3AIR on other platforms), no additional real file provider is needed, so this part is skipped
	if (config.mGameDataPath != L"data" && config.mGameDataPath != L"./data")
	{
		rmx::RealFileProvider* provider = new rmx::RealFileProvider();
		FTX::FileSystem->addManagedFileProvider(*provider);
		FTX::FileSystem->addMountPoint(*provider, L"data/", config.mGameDataPath + L'/', 0x10);
	}

	// Create mod data folder (the default mod directory)
	FTX::FileSystem->createDirectory(config.mGameAppDataPath + L"mods");

	// Add package providers
	if (!loadFilePackages(false))
		return false;
#if defined(PLATFORM_PS4)
	{
		int numLoaded = 0;
		for (PackedFileProvider* provider : mPackedFileProviders)
			numLoaded += (nullptr != provider) ? 1 : 0;
		PS4_STAGE("data packages loaded: %d of %d (game data path '%s')", numLoaded, (int)mPackedFileProviders.size(), WString(config.mGameDataPath).toStdString().c_str());
	}
#endif

	// Sanity check if engine data exists
	//  -> The Oxygen icon is a file that is always part of the engine data, so we just check for that
	if (!FTX::FileSystem->exists(config.mEngineDataPath + L"/oxygen_icon.png"))
	{
		PS4_STAGE("engine data NOT found ('%s/oxygen_icon.png')", WString(config.mEngineDataPath).toStdString().c_str());
		if (mDelegate.isDedicatedApplication())
			RMX_ERROR("Could not find engine data.\nThis can mean your game installation is broken and needs to be downloaded and installed again.\n\nIn case you manually replaced your data folder with the source data files, please make sure to also copy over the files from 'oxygenengine/data' as well.", )
		else
			RMX_ERROR("Could not find engine data.\nThis can mean your game installation is broken and needs to be downloaded and installed again.", );
		return false;
	}

	return true;
}

bool EngineMain::loadFilePackages(bool forceReload)
{
	Configuration& config = Configuration::instance();
	GameProfile& gameProfile = GameProfile::instance();
	mPackedFileProviders.resize(gameProfile.mDataPackages.size(), nullptr);

	for (size_t index = 0; index < gameProfile.mDataPackages.size(); ++index)
	{
		const bool success = loadFilePackageByIndex(index, forceReload);
		if (!success)
		{
			// Is this a required package after all?
			const GameProfile::DataPackage& dataPackage = gameProfile.mDataPackages[index];
			if (dataPackage.mRequired)
			{
				// We still accept missing packages if any data is present in unpacked form
				//  -> Just checking the "icon.png" to know whether that's the case
				static const bool hasUnpackedData = FTX::FileSystem->exists(config.mGameDataPath + L"/images/icon.png");
				RMX_CHECK(hasUnpackedData, "Could not find or open package '" << *WString(dataPackage.mFilename).toString() << "', application will close now again.", return false);
			}
		}
	}

	return true;
}

bool EngineMain::loadFilePackageByIndex(size_t index, bool forceReload)
{
	// Already loaded?
	if (nullptr != mPackedFileProviders[index])
	{
		if (forceReload)
		{
			FTX::FileSystem->destroyManagedFileProvider(*mPackedFileProviders[index]);
			mPackedFileProviders[index] = nullptr;
		}
		else
		{
			// Just ignore that one, it's already loaded
			return true;
		}
	}

	const GameProfile::DataPackage& dataPackage = GameProfile::instance().mDataPackages[index];
	Configuration& config = Configuration::instance();

	// First try loading from game installation
	const std::wstring gameDataBasePath = config.mGameDataPath + L"/";
	PackedFileProvider* provider = PackedFileProvider::createPackedFileProvider(gameDataBasePath + dataPackage.mFilename);
	if (nullptr == provider)
	{
		// Then try loading from save data (e.g. downloaded packages)
		const std::wstring saveDataBasePath = config.mAppDataPath + L"/data/";
		provider = PackedFileProvider::createPackedFileProvider(saveDataBasePath + dataPackage.mFilename);
	}
#if defined(PLATFORM_PS4)
	const char* ps4PackageSource = (nullptr != provider) ? "game data (/app0/data) or save data" : "not found";
	if (nullptr == provider)
	{
		// PS4: optional packages (like "audioremaster.bin", which the pkg does not carry by default) can be uploaded by the user to /data/sonic3air/ via FTP
		//  -> The package only gets its table of contents read here, entries are streamed from the file on demand
		provider = PackedFileProvider::createPackedFileProvider(L"/data/sonic3air/" + dataPackage.mFilename);
		if (nullptr != provider)
			ps4PackageSource = "/data/sonic3air/";
	}
	PS4_STAGE("data package '%s': %s", WString(dataPackage.mFilename).toStdString().c_str(), ps4PackageSource);
#endif

	if (nullptr != provider)
	{
		// Mount to "data" in any case, otherwise OxygenApp won't work when the game data path is somewhere different
		FTX::FileSystem->addManagedFileProvider(*provider);
		FTX::FileSystem->addMountPoint(*provider, L"data/", L"data/", 0x20 + (int)index);
		mPackedFileProviders[index] = provider;
		return true;
	}

	// Failed
	return false;
}

bool EngineMain::createWindow()
{
	Configuration& config = Configuration::instance();
	const EngineDelegateInterface::AppMetaData& appMetaData = mDelegate.getAppMetaData();

#if defined(PLATFORM_PS4)
	// Without the orbis SDL video driver (build option PS4_SDL_ORBIS_VIDEO=OFF) SDL runs its "dummy"
	// driver, which cannot create a GL context at all - and SDL_CreateWindow with SDL_WINDOW_OPENGL fails
	// outright there. Fall back to the software renderer so the rest of the engine still boots headless.
	{
		const char* videoDriver = SDL_GetCurrentVideoDriver();
		if (nullptr != videoDriver && SDL_strcmp(videoDriver, "dummy") == 0 && config.mRenderMethod != Configuration::RenderMethod::SOFTWARE)
		{
			PS4_STAGE("SDL video driver is 'dummy' (no GL): using the software renderer instead of render method %d", (int)config.mRenderMethod);
			config.mRenderMethod = Configuration::RenderMethod::SOFTWARE;
		}
	}
#endif

	const bool useOpenGL = (config.mRenderMethod == Configuration::RenderMethod::OPENGL_FULL) || (config.mRenderMethod == Configuration::RenderMethod::OPENGL_SOFT);

	// Setup video config
	rmx::VideoConfig videoConfig(config.mWindowMode != Configuration::WindowMode::WINDOWED, config.mWindowSize.x, config.mWindowSize.y, appMetaData.mTitle.c_str());
	videoConfig.mRenderer = useOpenGL ? rmx::VideoConfig::Renderer::OPENGL : rmx::VideoConfig::Renderer::SOFTWARE;
	videoConfig.mResizeable = true;
	videoConfig.mAutoClearScreen = useOpenGL;
	videoConfig.mAutoSwapBuffers = false;
	videoConfig.mVSync = (config.mFrameSync >= Configuration::FrameSyncType::VSYNC_ON);
	videoConfig.mIconResource = appMetaData.mWindowsIconResource;

	SDL_SetHint(SDL_HINT_RENDER_VSYNC, videoConfig.mVSync ? "1" : "0");

#if defined(LOAD_APP_ICON_PNG)
	// Load app icon
	if (!appMetaData.mIconFile.empty())
	{
		RMX_LOG_INFO("Loading application icon...");
		FileHelper::loadBitmap(videoConfig.mIconBitmap, appMetaData.mIconFile);
	}
#endif

	if (useOpenGL)
	{
		// Set SDL OpenGL attributes
		RMX_LOG_INFO("Setup of OpenGL attributes...");
	#if !defined(RMX_USE_GLES2)
		{
			// OpenGL 3.1 or 3.2
			const int majorVersion = 3;
		#if defined(PLATFORM_MAC)
			// macOS needs OpenGL 3.2 for GLSL 140 shaders to work. https://stackoverflow.com/a/31805596
			const int minorVersion = 2;
		#else
			const int minorVersion = 1;

			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		#endif

			RMX_LOG_INFO("Using OpenGL " << majorVersion << "." << minorVersion);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, majorVersion);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minorVersion);
		}
	#else
		{
			// GL ES 2.0
			const int majorVersion = 2;
			const int minorVersion = 0;

			RMX_LOG_INFO("Using OpenGL ES " << majorVersion << "." << minorVersion);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, majorVersion);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minorVersion);
		}
	#endif
	}

	// Create window
	{
		// TODO:
		//  - All of this needs a refactoring in general, as there's quite some overlap with "Application::setWindowMode"
		//  - There should also be a rework for the SDL3 behavior, which is kind of hacky right now

		const int displayIndex = config.mDisplayIndex;

		uint32 flags = useOpenGL ? SDL_WINDOW_OPENGL : 0;
		switch (config.mWindowMode)
		{
			case Configuration::WindowMode::WINDOWED:
			{
				// (Non-maximized) Window
				if (videoConfig.mResizeable)
					flags |= SDL_WINDOW_RESIZABLE;
				break;
			}

			case Configuration::WindowMode::FULLSCREEN_BORDERLESS:
			{
				// Borderless maximized window
				videoConfig.mWindowRect.setSize(getDisplaySize(displayIndex));
				flags |= SDL_WINDOW_BORDERLESS;
				break;
			}

			case Configuration::WindowMode::FULLSCREEN_DESKTOP:
			{
				// Fullscreen window at desktop resolution
				//  -> According to https://wiki.libsdl.org/SDL_SetWindowFullscreen, this is not really an exclusive fullscreen mode, but that's fine
				videoConfig.mWindowRect.setSize(getDisplaySize(displayIndex));
			#ifdef RMX_USE_SDL3
				flags |= SDL_WINDOW_FULLSCREEN;
			#else
				flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
			#endif
				break;
			}

			case Configuration::WindowMode::FULLSCREEN_EXCLUSIVE:
			{
				// Real exclusive fullscreen with desktop resolution (though also allowing for a custom resolution)
				videoConfig.mWindowRect.setSize(getDisplaySize(displayIndex));
				flags |= SDL_WINDOW_FULLSCREEN;
				break;
			}
		}

		RMX_LOG_INFO("Creating window...");
	#ifdef RMX_USE_SDL3
		{
			SDL_PropertiesID props = SDL_CreateProperties();
			SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, *videoConfig.mCaption);
			SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex));
			SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex));
			SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, videoConfig.mWindowRect.width);
			SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, videoConfig.mWindowRect.height);
			SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, flags);
			mSDLWindow = SDL_CreateWindowWithProperties(props);
			SDL_DestroyProperties(props);

			if (nullptr == mSDLWindow)
				return false;

			if (config.mWindowMode == Configuration::WindowMode::FULLSCREEN_EXCLUSIVE)
			{
				const SDL_DisplayID displayID = SDL_GetDisplayForWindow(mSDLWindow);
				const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(displayID);
				if (nullptr != mode)
				{
					SDL_SetWindowFullscreenMode(mSDLWindow, mode);
					SDL_SetWindowFullscreen(mSDLWindow, true);
				}
			}
		}
	#else
		mSDLWindow = SDL_CreateWindow(*videoConfig.mCaption, SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), videoConfig.mWindowRect.width, videoConfig.mWindowRect.height, flags);
		if (nullptr == mSDLWindow)
		{
			return false;
		}
	#endif

		RMX_LOG_INFO("Retrieving actual window size...");
		SDL_GetWindowSize(mSDLWindow, &videoConfig.mWindowRect.width, &videoConfig.mWindowRect.height);
		SDL_ShowCursor(!videoConfig.mHideCursor);

		if (useOpenGL)
		{
			RMX_LOG_INFO("Creating OpenGL context...");
			SDL_GLContext context = SDL_GL_CreateContext(mSDLWindow);
		#if defined(PLATFORM_PS4)
			mSDLGLContext = context;
		#endif
			if (nullptr != context)
			{
				RMX_LOG_INFO("Vsync setup...");
				setVSyncMode(config.mFrameSync);
			}
			else
			{
				RMX_LOG_INFO("Failed to create OpenGL context, fallback to pure software renderer");
				config.mRenderMethod = Configuration::RenderMethod::SOFTWARE;
				// TODO: In this case, the SDL window was created with SDL_WINDOW_OPENGL flag, but that does not seem to be a problem
			}
		}
	}

	// Create drawer depending on render method
#ifdef RMX_WITH_OPENGL_SUPPORT
	if (config.mRenderMethod >= Configuration::RenderMethod::OPENGL_SOFT)
	{
		if (!mDrawer.createDrawer<OpenGLDrawer>())
		{
			// Fallback to software drawer
			RMX_LOG_INFO("OpenGL drawer setup failed, using software rendering");
			config.mRenderMethod = Configuration::RenderMethod::SOFTWARE;
		#if defined(PLATFORM_PS4)
			// The software drawer's GLES2 renderer recreates the window - the unused GL context must not outlive it (see "destroyWindow")
			if (nullptr != mSDLGLContext)
			{
				SDL_GL_DeleteContext(mSDLGLContext);
				mSDLGLContext = nullptr;
			}
		#endif
			mDrawer.createDrawer<SoftwareDrawer>();
		}
	}
	else
#endif
	{
		mDrawer.createDrawer<SoftwareDrawer>();
	}

	// Tell FTX video manager that everything is okay
	FTX::Video->setInitialized(videoConfig, mSDLWindow);

#if defined(PLATFORM_WINDOWS)
	// Set window icon (using a Windows-specific method)
	if (videoConfig.mIconResource != 0)
	{
		RMX_LOG_INFO("Setting window icon (Windows)...");
		PlatformFunctions::setAppIcon(videoConfig.mIconResource);
	}
#endif

#if defined(LOAD_APP_ICON_PNG)
	// Set window icon (using SDL functionality)
	if (nullptr != videoConfig.mIconBitmap.getData() || videoConfig.mIconSource.nonEmpty())
	{
		RMX_LOG_INFO("Setting window icon from loaded bitmap...");
		Bitmap tmp;
		Bitmap* bitmap = &videoConfig.mIconBitmap;
		if (bitmap->empty())
		{
			bitmap = nullptr;
			if (tmp.load(videoConfig.mIconSource.toWString()))
			{
				bitmap = &tmp;
			}
		}

		if (nullptr != bitmap)
		{
			bitmap->rescale(32, 32);
		#ifdef RMX_USE_SDL3
			SDL_Surface* icon = SDL_CreateSurfaceFrom(32, 32, SDL_PIXELFORMAT_ABGR8888, bitmap->getData(), bitmap->getWidth() * sizeof(uint32));
		#else
			SDL_Surface* icon = SDL_CreateRGBSurfaceFrom(bitmap->getData(), 32, 32, 32, bitmap->getWidth() * sizeof(uint32), 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
		#endif
			SDL_SetWindowIcon(mSDLWindow, icon);
			SDL_FreeSurface(icon);
		}
	}
#endif

	return true;
}

void EngineMain::destroyWindow()
{
	mSystems.mVideoOut.destroyRenderer();
	mDrawer.destroyDrawer();
#if defined(PLATFORM_PS4)
	// PS4: the GL context has to go before the window, and before any new window gets created
	//  -> The console has one scan-out, and sceVideoOut allows one open handle. Mesa (zink/kopper) opens it with the swapchain of
	//     the window surface and closes it only when the last reference to that swapchain's back buffer is gone - and a context
	//     that rendered to the surface holds one until the context is destroyed. Leaking the context here (as the other platforms
	//     do) left the old swapchain alive on a render method switch, so the new window never presented anything.
	//  -> SDL_GL_DeleteContext releases the context first if it is current; it needs the EGL library that SDL_DestroyWindow unloads.
	if (nullptr != mSDLGLContext)
	{
		SDL_GL_DeleteContext(mSDLGLContext);
		mSDLGLContext = nullptr;
		PS4_STAGE("video: GL context deleted before window destruction");
	}
#endif
	SDL_DestroyWindow(mSDLWindow);
	mSDLWindow = nullptr;
}
