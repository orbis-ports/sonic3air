/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#ifdef RMX_WITH_OPENGL_SUPPORT

#include "oxygen/rendering/utils/BufferTexture.h"

#if defined(PLATFORM_PS4)
	#include "rmxbase/tools/PS4Stage.h"
	#include <cstdlib>
	#include <cstring>
#endif


// If buffer textures are not supported, we use normal textures instead
#if !defined(RMX_USE_GLES2)
	#define SUPPORTS_BUFFER_TEXTURES
#endif


#if defined(PLATFORM_PS4)
namespace
{
	int gPS4RoundRobinSlots = 3;
}

// PS4 (GL via zink + u_threaded_context on RADV): why this exists
//  -> The game re-uploads its scroll offset buffers every frame with same-size glBufferData, and its pattern cache with
//     glBufferSubData. The buffer is still referenced by the previous frame, which is usually still on the GPU.
//  -> Mesa then asks the driver "is this buffer busy?" (tc_improve_map_buffer_flags -> tc_is_buffer_busy -> zink
//     is_resource_busy -> vkWaitSemaphores(timeout 0)), and for a whole-buffer write a second time in tc_invalidate_buffer,
//     after which it allocates a new buffer and rebinds it everywhere. Each "busy" answer is a nonzero-deadline syncobj
//     wait on this platform that prints "syncobj wait timed out" at warning level to stderr = klog (8-15 ms a line).
//  -> Round-robin writes into a buffer the GPU is done with: the busy check answers "idle" (no warning), the write is
//     mapped unsynchronized, and there is no invalidation/realloc/rebind at all.
BufferTexture::UploadStrategy BufferTexture::getUploadStrategy()
{
	static const UploadStrategy strategy = []()
	{
		UploadStrategy result = UploadStrategy::ROUND_ROBIN;
		const char* value = getenv("S3AIR_BUFFER_UPLOAD");
		if (nullptr != value)
		{
			if (strcmp(value, "static") == 0 || strcmp(value, "subdata") == 0 || strcmp(value, "old") == 0)
				result = UploadStrategy::STATIC;
			else if (strcmp(value, "orphan") == 0)
				result = UploadStrategy::ORPHAN;
			else if (strcmp(value, "rr") == 0 || strcmp(value, "roundrobin") == 0)
				result = UploadStrategy::ROUND_ROBIN;
			else
				ps4_log("S3AIR_BUFFER_UPLOAD: unknown value '%s' - using the default 'rr'", value);
		}
		if (const char* slots = getenv("S3AIR_BUFFER_RR_SLOTS"))
		{
			gPS4RoundRobinSlots = clamp(atoi(slots), 2, MAX_SLOTS);
		}
		ps4_log("S3AIR_BUFFER_UPLOAD: %s (%s), round-robin slots %d", (result == UploadStrategy::STATIC) ? "static" : (result == UploadStrategy::ORPHAN) ? "orphan" : "rr",
				(nullptr == value) ? "default" : "from env", gPS4RoundRobinSlots);
		return result;
	}();
	return strategy;
}
#endif


bool BufferTexture::supportsBufferTextures()
{
#if defined(SUPPORTS_BUFFER_TEXTURES)
	return true;
#else
	return false;
#endif
}

BufferTexture::BufferTexture()
{
}

BufferTexture::~BufferTexture()
{
#if defined(SUPPORTS_BUFFER_TEXTURES) && defined(PLATFORM_PS4)
	if (mNumSlots > 0)
	{
		// mTextureHandle / mTexBuffer only alias the current slot
		for (int k = 0; k < mNumSlots; ++k)
		{
			if (mSlots[k].mBuffer != (GLuint)~0)
				glDeleteBuffers(1, &mSlots[k].mBuffer);
			if (mSlots[k].mTexture != 0)
				glDeleteTextures(1, &mSlots[k].mTexture);
		}
		return;
	}
#endif

#if defined(SUPPORTS_BUFFER_TEXTURES)
	if (mTexBuffer != (GLuint)~0)
	{
		glDeleteBuffers(1, &mTexBuffer);
	}
#endif

	if (mTextureHandle != 0)
	{
		glDeleteTextures(1, &mTextureHandle);
	}
}

void BufferTexture::create(PixelFormat pixelFormat, int width, int height, const void* data)
{
	// Only 1 or 2 bytes supported at the moment
	mPixelFormat = pixelFormat;

#if defined(SUPPORTS_BUFFER_TEXTURES) && defined(PLATFORM_PS4)
	if (getUploadStrategy() == UploadStrategy::ROUND_ROBIN && (mNumSlots > 0 || mTextureHandle == 0))
	{
		mSize.set(width, height);
		if (mNumSlots == 0)
		{
			mNumSlots = gPS4RoundRobinSlots;
			for (int k = 0; k < mNumSlots; ++k)
			{
				glGenTextures(1, &mSlots[k].mTexture);
				glGenBuffers(1, &mSlots[k].mBuffer);
			}
		}
		mByteSize = -1;
		respecifyAllSlots(data, (width > 0 && height > 0) ? width * height * ((mPixelFormat == PixelFormat::UINT_8) ? 1 : 2) : 0);

		const GLint internalFormat = (mPixelFormat == PixelFormat::UINT_8) ? GL_R8UI : (mPixelFormat == PixelFormat::INT_16) ? GL_R16I : GL_R16UI;
		for (int k = 0; k < mNumSlots; ++k)
		{
			glBindTexture(GL_TEXTURE_BUFFER, mSlots[k].mTexture);
			glTexBuffer(GL_TEXTURE_BUFFER, internalFormat, mSlots[k].mBuffer);
		}
		glBindTexture(GL_TEXTURE_BUFFER, 0);

		mCurrentSlot = 0;
		mTextureHandle = mSlots[0].mTexture;
		mTexBuffer = mSlots[0].mBuffer;
		return;
	}
#endif

	if (mTextureHandle == 0)
	{
		glGenTextures(1, &mTextureHandle);
	}

	mSize.set(width, height);

#if defined(SUPPORTS_BUFFER_TEXTURES)
	glGenBuffers(1, &mTexBuffer);
	if (width > 0 && height > 0)
	{
		bufferData(data, width, height);
	}

	const GLint internalFormat = (mPixelFormat == PixelFormat::UINT_8) ? GL_R8UI : (mPixelFormat == PixelFormat::INT_16) ? GL_R16I : GL_R16UI;
	glBindTexture(GL_TEXTURE_BUFFER, mTextureHandle);
	glTexBuffer(GL_TEXTURE_BUFFER, internalFormat, mTexBuffer);
	glBindTexture(GL_TEXTURE_BUFFER, 0);
#else
	const GLint internalFormat = (mPixelFormat == PixelFormat::UINT_8) ? GL_LUMINANCE : GL_LUMINANCE_ALPHA;
	glBindTexture(GL_TEXTURE_2D, mTextureHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, internalFormat, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
#endif
}

void BufferTexture::create(PixelFormat pixelFormat, Vec2i size, const void* data)
{
	create(pixelFormat, size.x, size.y, data);
}

void BufferTexture::bindBuffer() const
{
#if defined(SUPPORTS_BUFFER_TEXTURES)
	glBindBuffer(GL_TEXTURE_BUFFER, mTexBuffer);
#else
	glBindTexture(GL_TEXTURE_2D, mTextureHandle);
#endif
}

void BufferTexture::unbindBuffer()
{
#if defined(SUPPORTS_BUFFER_TEXTURES)
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
#else
	glBindTexture(GL_TEXTURE_2D, 0);
#endif
}

void BufferTexture::bindTexture() const
{
#if defined(SUPPORTS_BUFFER_TEXTURES)
	glBindTexture(GL_TEXTURE_BUFFER, mTextureHandle);
#else
	glBindTexture(GL_TEXTURE_2D, mTextureHandle);
#endif
}

void BufferTexture::bufferData(const void* data, int width, int height)
{
#if defined(SUPPORTS_BUFFER_TEXTURES)
	const int bytesPerPixel = (mPixelFormat == PixelFormat::UINT_8) ? 1 : 2;
#if defined(PLATFORM_PS4)
	const int byteSize = width * height * bytesPerPixel;
	if (mNumSlots > 0)
	{
		if (byteSize != mByteSize || nullptr == data)
		{
			// New size: fresh storage everywhere, nothing in flight to wait for
			respecifyAllSlots(data, byteSize);
			return;
		}
		if ((int)mShadow.size() == byteSize && memcmp(mShadow.data(), data, (size_t)byteSize) == 0)
		{
			// Unchanged since the last upload: every slot is up to date or has it marked dirty already
			return;
		}
		updateRange(data, 0, byteSize);
		mShadow.assign((const uint8*)data, (const uint8*)data + byteSize);
		return;
	}
	if (getUploadStrategy() == UploadStrategy::ORPHAN)
	{
		glBindBuffer(GL_TEXTURE_BUFFER, mTexBuffer);
		glBufferData(GL_TEXTURE_BUFFER, byteSize, nullptr, GL_STREAM_DRAW);
		if (nullptr != data && byteSize > 0)
			glBufferSubData(GL_TEXTURE_BUFFER, 0, byteSize, data);
		glBindBuffer(GL_TEXTURE_BUFFER, 0);
		return;
	}
#endif
	glBindBuffer(GL_TEXTURE_BUFFER, mTexBuffer);
	glBufferData(GL_TEXTURE_BUFFER, width * height * bytesPerPixel, data, GL_STATIC_DRAW);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
#else
	const GLint internalFormat = (mPixelFormat == PixelFormat::UINT_8) ? GL_LUMINANCE : GL_LUMINANCE_ALPHA;
	glBindTexture(GL_TEXTURE_2D, mTextureHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, internalFormat, GL_UNSIGNED_BYTE, data);
	glBindTexture(GL_TEXTURE_2D, 0);
#endif
}

#if defined(PLATFORM_PS4)
void BufferTexture::updateRange(const void* fullData, int offset, int size)
{
	if (size <= 0)
		return;

#if defined(SUPPORTS_BUFFER_TEXTURES)
	if (mNumSlots == 0)
	{
		// Not round-robin: plain partial update of the one buffer
		glBindBuffer(GL_TEXTURE_BUFFER, mTexBuffer);
		glBufferSubData(GL_TEXTURE_BUFFER, offset, size, (const uint8*)fullData + offset);
		glBindBuffer(GL_TEXTURE_BUFFER, 0);
		return;
	}

	const int end = offset + size;
	RMX_ASSERT(offset >= 0 && end <= mByteSize, "Invalid buffer texture update range");

	// Every slot misses this change now
	for (int k = 0; k < mNumSlots; ++k)
	{
		Slot& slot = mSlots[k];
		if (slot.mDirtyBegin >= slot.mDirtyEnd)
		{
			slot.mDirtyBegin = offset;
			slot.mDirtyEnd = end;
		}
		else
		{
			slot.mDirtyBegin = std::min(slot.mDirtyBegin, offset);
			slot.mDirtyEnd = std::max(slot.mDirtyEnd, end);
		}
	}

	// Write into the least recently used slot, i.e. the one the GPU finished with longest ago, including what it missed so far
	const int nextSlot = (mCurrentSlot + 1) % mNumSlots;
	Slot& slot = mSlots[nextSlot];
	glBindBuffer(GL_TEXTURE_BUFFER, slot.mBuffer);
	glBufferSubData(GL_TEXTURE_BUFFER, slot.mDirtyBegin, slot.mDirtyEnd - slot.mDirtyBegin, (const uint8*)fullData + slot.mDirtyBegin);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
	slot.mDirtyBegin = slot.mDirtyEnd = 0;

	if ((int)mShadow.size() == mByteSize)
		memcpy(&mShadow[offset], (const uint8*)fullData + offset, (size_t)size);

	mCurrentSlot = nextSlot;
	mTextureHandle = slot.mTexture;
	mTexBuffer = slot.mBuffer;
#else
	// Normal textures: not used on PS4
	bufferData(fullData, mSize.x, mSize.y);
#endif
}

void BufferTexture::respecifyAllSlots(const void* data, int byteSize)
{
	for (int k = 0; k < mNumSlots; ++k)
	{
		glBindBuffer(GL_TEXTURE_BUFFER, mSlots[k].mBuffer);
		glBufferData(GL_TEXTURE_BUFFER, byteSize, data, GL_STATIC_DRAW);
		mSlots[k].mDirtyBegin = mSlots[k].mDirtyEnd = 0;
	}
	glBindBuffer(GL_TEXTURE_BUFFER, 0);

	mByteSize = byteSize;
	if (nullptr != data && byteSize > 0)
		mShadow.assign((const uint8*)data, (const uint8*)data + byteSize);
	else
		mShadow.clear();	// Contents undefined: never skip the next upload
}
#endif

#endif
