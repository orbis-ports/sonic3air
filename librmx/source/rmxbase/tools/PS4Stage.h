/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

// Boot stage markers for the PlayStation 4 port.
//  -> Each marker is one netlog line "[sonic3air] S3AIR_STAGE <text>", received on the dev host by
//     orbis-compat/scripts/ps4/logs.sh (UDP 18194). On a console without a working netlog the line
//     goes to klog instead (ps4_app.cpp decides that).
//  -> On every other platform the macro compiles to nothing (and PS4FrameTelemetry does not exist).
//  -> Include this after "rmxbase.h" (it needs PLATFORM_PS4 to be decided already).

#if defined(PLATFORM_PS4)
	#include <ps4_app.h>
	#include <chrono>
	#include <cstdint>
	#define PS4_STAGE(fmt, ...)		ps4_log("S3AIR_STAGE " fmt, ##__VA_ARGS__)

	// Frame-time telemetry for the console (stutter hunt).
	//  -> Everything is accumulated per frame without any output; one netlog line (UDP only, ps4_log_frame)
	//     every REPORT_FRAMES presents. Never log per frame: klog costs 8-15 ms a line on this console.
	//  -> Main thread only (Application::render and the renderer it calls), so no synchronization.
	struct PS4FrameTelemetry
	{
		static constexpr uint32_t REPORT_FRAMES = 600;

		static inline uint64_t nowUs()
		{
			return (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		}

		static inline void addUpload(uint64_t us)		{ if (us > mUploadMaxUs) mUploadMaxUs = us; mUploadSumUs += us; }
		static inline void addRender(uint64_t us)		{ if (us > mRenderMaxUs) mRenderMaxUs = us; }
		static inline void addSleep(uint64_t us)		{ if (us > mSleepMaxUs) mSleepMaxUs = us; }

		// Call right after the buffer swap returned
		static inline void framePresented(uint64_t swapStartUs, uint64_t swapEndUs)
		{
			const uint64_t swapUs = swapEndUs - swapStartUs;
			if (swapUs > mSwapMaxUs)
				mSwapMaxUs = swapUs;
			if (mLastPresentUs != 0)
			{
				const uint64_t frameUs = swapEndUs - mLastPresentUs;
				mFrameSumUs += frameUs;
				if (frameUs > mFrameMaxUs)
					mFrameMaxUs = frameUs;
				if (frameUs > 20000)
					++mOver20;
				if (frameUs > 34000)
					++mOver34;
				++mFrames;
			}
			mLastPresentUs = swapEndUs;

			if (mFrames >= REPORT_FRAMES)
			{
				ps4_log_frame("S3AIR_FRAMES %u frames: avg %.2f ms, max %.1f ms, >20ms %u, >34ms %u | swap max %.1f ms | render+sleep max %.1f ms | upload max %.2f ms avg %.3f ms | sleep max %.1f ms (%s)",
							  (unsigned)mFrames, (double)mFrameSumUs / (double)mFrames / 1000.0, (double)mFrameMaxUs / 1000.0, (unsigned)mOver20, (unsigned)mOver34,
							  (double)mSwapMaxUs / 1000.0, (double)mRenderMaxUs / 1000.0, (double)mUploadMaxUs / 1000.0, (double)mUploadSumUs / (double)mFrames / 1000.0,
							  (double)mSleepMaxUs / 1000.0, (mFrameSyncMode == 1) ? "framecap" : "vsync");
				mFrames = 0;
				mFrameSumUs = mFrameMaxUs = mSwapMaxUs = mRenderMaxUs = mUploadMaxUs = mUploadSumUs = mSleepMaxUs = 0;
				mOver20 = mOver34 = 0;
			}
		}

		static inline uint64_t mLastPresentUs = 0;
		static inline uint64_t mFrameSumUs = 0;
		static inline uint64_t mFrameMaxUs = 0;
		static inline uint64_t mSwapMaxUs = 0;
		static inline uint64_t mRenderMaxUs = 0;
		static inline uint64_t mUploadMaxUs = 0;
		static inline uint64_t mUploadSumUs = 0;
		static inline uint64_t mSleepMaxUs = 0;
		static inline uint32_t mFrames = 0;
		static inline uint32_t mOver20 = 0;
		static inline uint32_t mOver34 = 0;
		static inline int mFrameSyncMode = 0;		// 0 = V-Sync path with SDL_Delay(1), 1 = frame cap with preciseDelay
	};
#else
	#define PS4_STAGE(fmt, ...)		((void)0)
#endif
