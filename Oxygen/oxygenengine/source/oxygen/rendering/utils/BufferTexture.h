/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#ifdef RMX_WITH_OPENGL_SUPPORT

#include <rmxmedia.h>


class BufferTexture
{
public:
	static bool supportsBufferTextures();

public:
	enum class PixelFormat
	{
		UINT_8,
		INT_16,
		UINT_16
	};

#if defined(PLATFORM_PS4)
	// How per-frame uploads reach the GPU, chosen once via env "S3AIR_BUFFER_UPLOAD" (see BufferTexture.cpp)
	enum class UploadStrategy
	{
		STATIC,			// Original behavior: glBufferData(..., GL_STATIC_DRAW) / partial glBufferSubData on the one buffer
		ORPHAN,			// glBufferData(size, NULL, GL_STREAM_DRAW) + full glBufferSubData
		ROUND_ROBIN		// N texture/buffer pairs, every upload goes into the least recently used one; unchanged data is skipped
	};
	static UploadStrategy getUploadStrategy();
#endif

public:
	BufferTexture();
	~BufferTexture();

	void create(PixelFormat pixelFormat, int width = 0, int height = 1, const void* data = nullptr);
	void create(PixelFormat pixelFormat, Vec2i size, const void* data = nullptr);

	inline bool isValid() const				{ return mTextureHandle != 0; }
	inline GLuint getTextureHandle() const	{ return mTextureHandle; }
	inline GLuint getBufferHandle() const	{ return mTexBuffer; }
	inline Vec2i getSize() const			{ return mSize; }

	void bindBuffer() const;
	static void unbindBuffer();

	void bindTexture() const;

	void bufferData(const void* data, int width, int height);

#if defined(PLATFORM_PS4)
	// Update bytes [offset, offset + size) - "fullData" must point to the complete buffer contents (not only the range),
	// because with round-robin buffers a slot may also need changes it missed while other slots were current
	void updateRange(const void* fullData, int offset, int size);
#endif

private:
	GLuint mTextureHandle = 0;
	GLuint mTexBuffer = (GLuint)~0;
	PixelFormat mPixelFormat = PixelFormat::UINT_8;
	Vec2i mSize;

#if defined(PLATFORM_PS4)
	static constexpr int MAX_SLOTS = 4;
	struct Slot
	{
		GLuint mTexture = 0;
		GLuint mBuffer = (GLuint)~0;
		int mDirtyBegin = 0;	// Byte range this slot has not received yet (empty if begin >= end)
		int mDirtyEnd = 0;
	};
	Slot mSlots[MAX_SLOTS];
	int mNumSlots = 0;			// 0 unless round-robin uploads are used
	int mCurrentSlot = 0;
	int mByteSize = -1;
	std::vector<uint8> mShadow;	// Last uploaded contents (round-robin only), to skip unchanged uploads

	void respecifyAllSlots(const void* data, int byteSize);
#endif
};

#endif
