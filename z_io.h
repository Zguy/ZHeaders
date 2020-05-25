/*
z_io - I/O library for reading/writing files or memory

USAGE
	#define Z_IO_IMPLEMENTATION
	before you include this file in *one* C or C++ file to create the implementation.

	#define Z_IO_STATIC
	before you include this file to create a private implementation.

EXAMPLE
	// Allocated on stack
	ZIOHandle handle;
	zio_open_file(&handle, filename, ZIOM_READ);
	zio_close(&handle);

	// Allocated on heap
	ZIOHandle *handle = (ZIOHandle*)malloc(sizeof(ZIOHandle));
	zio_open_file(handle, filename, ZIOM_READ);
	zio_close(handle);
	free(handle);

UNLICENSE
	This is free and unencumbered software released into the public domain.

	Anyone is free to copy, modify, publish, use, compile, sell, or
	distribute this software, either in source code form or as a compiled
	binary, for any purpose, commercial or non-commercial, and by any
	means.

	In jurisdictions that recognize copyright laws, the author or authors
	of this software dedicate any and all copyright interest in the
	software to the public domain. We make this dedication for the benefit
	of the public at large and to the detriment of our heirs and
	successors. We intend this dedication to be an overt act of
	relinquishment in perpetuity of all present and future rights to this
	software under copyright law.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
	OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
	ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.

	For more information, please refer to <http://unlicense.org>
*/
#ifndef ZIO_INCLUDED_IO_H
#define ZIO_INCLUDED_IO_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef Z_IO_STATIC
	#define ZIODEF static
#else
	#define ZIODEF extern
#endif

typedef long long zio_ll;
typedef int zio_result;

typedef enum
{
	ZIOM_WRITE  = 1<<0,
	ZIOM_READ   = 1<<1,
} ZIOMode;

typedef enum
{
	ZIO_SEEK_SET, // Seek from the beginning of the data
	ZIO_SEEK_CUR, // Seek from the current position
	ZIO_SEEK_END, // Seek relative to the end of the data
} ZIOSeek;

enum
{
	ZIO_ERROR = -1,
	ZIO_OK    = 0,
};

typedef struct ZIOHandle ZIOHandle;

struct ZIOHandle
{
	zio_result (*close)(ZIOHandle *handle);
	zio_ll (*size)(ZIOHandle *handle);
	zio_ll (*seek)(ZIOHandle *handle, zio_ll offset, ZIOSeek whence);
	zio_ll (*read)(ZIOHandle *handle, void *destination, zio_ll size);
	zio_ll (*write)(ZIOHandle *handle, const void *source, zio_ll size);

	const char *last_error;

	union
	{
		struct
		{
			void *handle;
		} file;
		struct
		{
			char *begin;
			char *pos;
			char *end;
		} mem;
	} data;
};

// 'handle' can be either malloc'ed or simply created on the stack
ZIODEF zio_result zio_open_file(ZIOHandle *handle, const char *filename, ZIOMode mode);
ZIODEF zio_result zio_open_memory(ZIOHandle *handle, void *memory, zio_ll size);
ZIODEF zio_result zio_open_const_memory(ZIOHandle *handle, const void *memory, zio_ll size);

static inline zio_result zio_close(ZIOHandle *handle) { return handle->close(handle); }

// Returns size of data, or ZIO_ERROR
static inline zio_ll zio_size(ZIOHandle *handle) { return handle->size(handle); }

// Returns position in data after seek, or ZIO_ERROR
static inline zio_ll zio_seek(ZIOHandle *handle, zio_ll offset, ZIOSeek whence) { return handle->seek(handle, offset, whence); }

// Returns position in data, or ZIO_ERROR
static inline zio_ll zio_tell(ZIOHandle *handle) { return handle->seek(handle, 0, ZIO_SEEK_CUR); }

// Returns bytes read, or ZIO_ERROR
static inline zio_ll zio_read(ZIOHandle *handle, void *destination, zio_ll size) { return handle->read(handle, destination, size); }

// Return bytes written, or ZIO_ERROR
static inline zio_ll zio_write(ZIOHandle *handle, const void *source, zio_ll size) { return handle->write(handle, source, size); }

static inline const char *zio_last_error(ZIOHandle *handle) { return handle->last_error; }

#ifdef __cplusplus
}
#endif

#endif // ZIO_INCLUDED_IO_H

#ifdef Z_IO_IMPLEMENTATION

#include <errno.h>
#include <stdio.h>
#include <string.h>

static inline void zio__zero_handle(ZIOHandle *handle)
{
	memset(handle, 0, sizeof(ZIOHandle));
}

static inline int zio__test_flag(int flags, int test)
{
	return ((flags & test) == test);
}

static inline zio_result zio__set_error(ZIOHandle *handle, const char *error_string)
{
	handle->last_error = error_string;
	return ZIO_ERROR;
}

// File I/O
static zio_result zio__file_close(ZIOHandle *handle)
{
	if (fclose(handle->data.file.handle) != 0)
		return zio__set_error(handle, strerror(errno));
	zio__zero_handle(handle);
	return ZIO_OK;
}
static zio_ll zio__file_size(ZIOHandle *handle)
{
	zio_ll pos = zio_tell(handle);
	if (pos == ZIO_ERROR)
		return ZIO_ERROR;
	zio_ll size = zio_seek(handle, 0, ZIO_SEEK_END);
	zio_seek(handle, pos, ZIO_SEEK_SET);
	return size;
}
static zio_ll zio__file_seek(ZIOHandle *handle, zio_ll offset, ZIOSeek whence)
{
	if (fseek(handle->data.file.handle, offset, whence) != 0)
		return zio__set_error(handle, strerror(errno));
	return ftell(handle->data.file.handle);
}
static zio_ll zio__file_read(ZIOHandle *handle, void *destination, zio_ll size)
{
	zio_ll read_count = fread(destination, size, 1, handle->data.file.handle);
	if (read_count == 0 && ferror(handle->data.file.handle))
		return zio__set_error(handle, strerror(errno));
	return read_count * size;
}
static zio_ll zio__file_write(ZIOHandle *handle, const void *source, zio_ll size)
{
	zio_ll write_count = fwrite(source, size, 1, handle->data.file.handle);
	if (write_count == 0 && ferror(handle->data.file.handle))
		return zio__set_error(handle, strerror(errno));;
	return write_count * size;
}

// Memory I/O
static zio_result zio__memory_close(ZIOHandle *handle)
{
	zio__zero_handle(handle);
	return ZIO_OK;
}
static zio_ll zio__memory_size(ZIOHandle *handle)
{
	zio_ll size = (handle->data.mem.end - handle->data.mem.begin);
	return size;
}
static zio_ll zio__memory_seek(ZIOHandle *handle, zio_ll offset, ZIOSeek whence)
{
	char *new_pos;
	switch (whence)
	{
	case ZIO_SEEK_SET:
		new_pos = handle->data.mem.begin + offset;
		break;
	case ZIO_SEEK_CUR:
		new_pos = handle->data.mem.pos + offset;
		break;
	case ZIO_SEEK_END:
		new_pos = handle->data.mem.end + offset;
		break;
	default:
		return zio__set_error(handle, "Invalid whence value");
	}

	if (new_pos < handle->data.mem.begin)
		new_pos = handle->data.mem.begin;
	if (new_pos > handle->data.mem.end)
		new_pos = handle->data.mem.end;

	handle->data.mem.pos = new_pos;
	zio_ll pos = (handle->data.mem.pos - handle->data.mem.begin);
	return pos;
}
static zio_ll zio__memory_read(ZIOHandle *handle, void *destination, zio_ll size)
{
	if (size <= 0)
		return zio__set_error(handle, "Invalid size");

	zio_ll mem_available = (handle->data.mem.end - handle->data.mem.pos);
	zio_ll total_bytes = size;
	if (total_bytes > mem_available)
		total_bytes = mem_available;

	memcpy(destination, handle->data.mem.pos, total_bytes);
	handle->data.mem.pos += total_bytes;
	return total_bytes;
}
static zio_ll zio__memory_write(ZIOHandle *handle, const void *source, zio_ll size)
{
	if (size <= 0)
		return zio__set_error(handle, "Invalid size");

	zio_ll mem_available = (handle->data.mem.end - handle->data.mem.pos);
	zio_ll total_bytes = size;
	if (total_bytes > mem_available)
		total_bytes = mem_available;

	memcpy(handle->data.mem.pos, source, total_bytes);
	handle->data.mem.pos += total_bytes;
	return total_bytes;
}
static zio_ll zio__const_memory_write(ZIOHandle *handle, const void *source, zio_ll size)
{
	return zio__set_error(handle, "Cannot write to const memory");
}

ZIODEF zio_result zio_open_file(ZIOHandle *handle, const char *filename, ZIOMode mode)
{
	char mode_flags[7];

	// Build mode string
	{
		int i = 0;
		if (zio__test_flag(mode, ZIOM_WRITE))
			mode_flags[i++] = 'w';
		else if (zio__test_flag(mode, ZIOM_READ))
			mode_flags[i++] = 'r';
		if (zio__test_flag(mode, ZIOM_WRITE | ZIOM_READ))
			mode_flags[i++] = '+';
		mode_flags[i++] = 'b'; // Always open in binary mode
		mode_flags[i] = '\0';
	}

	zio__zero_handle(handle);

	FILE *file = fopen(filename, mode_flags);
	if (!file)
		return zio__set_error(handle, strerror(errno));

	handle->data.file.handle = file;

	handle->close = zio__file_close;
	handle->size  = zio__file_size;
	handle->seek  = zio__file_seek;
	handle->read  = zio__file_read;
	handle->write = zio__file_write;
	return ZIO_OK;
}

ZIODEF zio_result zio_open_memory(ZIOHandle *handle, void *memory, zio_ll size)
{
	zio__zero_handle(handle);

	if (!memory || size < 0)
		return zio__set_error(handle, "Invalid memory or size");

	handle->data.mem.begin = (char*)memory;
	handle->data.mem.pos = handle->data.mem.begin;
	handle->data.mem.end = handle->data.mem.begin + size;

	handle->close = zio__memory_close;
	handle->size  = zio__memory_size;
	handle->seek  = zio__memory_seek;
	handle->read  = zio__memory_read;
	handle->write = zio__memory_write;
	return ZIO_OK;
}

ZIODEF zio_result zio_open_const_memory(ZIOHandle *handle, const void *memory, zio_ll size)
{
	zio__zero_handle(handle);

	if (!memory || size < 0)
		return zio__set_error(handle, "Invalid memory or size");

	handle->data.mem.begin = (char*)memory;
	handle->data.mem.pos = handle->data.mem.begin;
	handle->data.mem.end = handle->data.mem.begin + size;

	handle->close = zio__memory_close;
	handle->size  = zio__memory_size;
	handle->seek  = zio__memory_seek;
	handle->read  = zio__memory_read;
	handle->write = zio__const_memory_write;
	return ZIO_OK;
}

#endif // Z_IO_IMPLEMENTATION
