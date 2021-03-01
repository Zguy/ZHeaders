/*
z_filesystem - Filesystem functions

PLATFORMS
Supports Windows and Linux at the moment, but Mac support should not be hard to implement.

USAGE
#define Z_FS_IMPLEMENTATION
before you include this file in *one* C or C++ file to create the implementation.

#define Z_FS_STATIC
before you include this file to create a private implementation.

#define Z_FS_NO_PATH
to disable path functions.
If directory traversal is enabled, path functions will be included anyway,
but as internal (static) functions.

#define Z_FS_NO_FILE
to disable file functions.

#define Z_FS_NO_DIRECTORY
to disable directory traversal functions.

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

// EXAMPLE
#if 0
char path[50];
zfs_path_full(path, sizeof(path), "file.txt");
zfs_file_touch(path);
zfs_file_delete(path);

char dir_path[50];
zfs_path_directory(dir_path, sizeof(dir_path), path);

ZFSDir dir;
if (zfs_directory_begin(&dir, dir_path))
{
	do
	{
		zfs_directory_current_filename(&dir, path, sizeof(path));
		zfs_bool is_dir = zfs_directory_is_directory(&dir);
	} while (zfs_directory_next(&dir));
	zfs_directory_end(&dir);
}
#endif

#ifndef ZFS_INCLUDED_FILESYSTEM_H
#define ZFS_INCLUDED_FILESYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef Z_FS_STATIC
#define ZFSDEF static
#else
#define ZFSDEF extern
#endif

#if defined(__linux)
#define ZFS_POSIX
#elif defined(_WIN32)
#define ZFS_WINDOWS
#else
#error Unsupported platform
#endif

	typedef long long zfs_ll;
	typedef int zfs_bool;

	enum
	{
		ZFS_FALSE,
		ZFS_TRUE,
	};

	// Path functions
#ifndef Z_FS_NO_PATH
	// Joins two paths.
	ZFSDEF void zfs_path_join(char *result, zfs_ll result_size, const char *left, const char *right);

	// Returns the extension of a file including the period (".txt").
	ZFSDEF void zfs_path_extension(char *result, zfs_ll result_size, const char *path);

	// Returns the path as-is, but with a replaced extension
	ZFSDEF void zfs_path_set_extension(char *result, zfs_ll result_size, const char *path, const char *new_extension);

	// Returns the file part of a path, given "/path/to/file.txt" it returns "file.txt".
	ZFSDEF void zfs_path_basename(char *result, zfs_ll result_size, const char *path);

	// Returns the file part of a path without the extension, given "/path/to/file.txt" it returns "file".
	ZFSDEF void zfs_path_basename_without_extension(char *result, zfs_ll result_size, const char *path);

	// Returns the directory part of a path including a trailing /, given "/path/to/file.txt" it returns "/path/to/".
	ZFSDEF void zfs_path_directory(char *result, zfs_ll result_size, const char *path);

	// Replaces all directory separators with the native separator. \ on Windows, / on Linux.
	ZFSDEF void zfs_path_normalize_inplace(char *path);

	// Replaces all directory separators with the native separator. \ on Windows, / on Linux. Operates on a copy of the string.
	ZFSDEF void zfs_path_normalize(char *result, zfs_ll result_size, const char *path);

	// Sets 'result' buffer to the current working directory.
	// Returns false if the current working directory could not be found.
	ZFSDEF zfs_bool zfs_path_working_directory(char *result, zfs_ll result_size);

	// If 'path' is a full path, it returns 'path', otherwise it joins 'path' with the working directory.
	ZFSDEF void zfs_path_full(char *result, zfs_ll result_size, const char *path);
#endif // Z_FS_NO_PATH

	// File functions
#ifndef Z_FS_NO_FILE
	// If 'filename' exists, the access and modified times are updated, otherwise the file is created.
	// Returns false if it failed.
	ZFSDEF zfs_bool zfs_file_touch(const char *filename);

	// Returns whether 'filename' exists or not.
	ZFSDEF zfs_bool zfs_file_exists(const char *filename);

	// Renames 'old_filename' to 'new_filename'.
	// Returns false if it failed.
	ZFSDEF zfs_bool zfs_file_rename(const char *old_filename, const char *new_filename);

	// Copies 'source_filename' to 'destination_filename'.
	// Copies in 32k byte chunks to conserve memory.
	// Returns false if it failed.
	ZFSDEF zfs_bool zfs_file_copy(const char *source_filename, const char *destination_filename);

	// Deletes 'filename'.
	// Returns false if it failed.
	ZFSDEF zfs_bool zfs_file_delete(const char *filename);
#endif // Z_FS_NO_FILE

	// Directory traversal
#ifndef Z_FS_NO_DIRECTORY
	typedef struct ZFSDir
	{
		void *handle;
#if defined(ZFS_POSIX)
		void *data;
#elif defined(ZFS_WINDOWS)
		char data[320]; // sizeof(WIN32_FIND_DATAA) == 320
#endif
	} ZFSDir;

	// Initializes directory traversal.
	// 'path' can contain a trailing / or not, but should not include "/*".
	// 'context' can be either malloc'ed or simply created on the stack
	// Returns false if it failed.
	ZFSDEF zfs_bool zfs_directory_begin(ZFSDir *context, const char *path);

	// Steps the directory traversal forward.
	// Returns false if we've reached the end.
	ZFSDEF zfs_bool zfs_directory_next(ZFSDir *context);

	// Cleans up the context.
	// Does not need to be called if zfs_directory_begin() returned false.
	ZFSDEF void zfs_directory_end(ZFSDir *context);

	// Returns the filename of the file or directory the context currently points at.
	ZFSDEF void zfs_directory_current_filename(ZFSDir *context, char *result, zfs_ll result_size);

	// Returns whether the context currently points at a directory.
	ZFSDEF zfs_bool zfs_directory_is_directory(ZFSDir *context);
#endif // Z_FS_NO_DIRECTORY

#ifdef __cplusplus
}
#endif

#endif // ZFS_INCLUDED_FILESYSTEM_H

#ifdef Z_FS_IMPLEMENTATION

#include <stdio.h> // For FILE API
#include <string.h> // For memcpy, strlen, strcmp

#if defined(ZFS_POSIX)
#include <dirent.h> // For directory walking API
#include <sys/time.h> // For utimes
#include <sys/stat.h> // For stat
#include <unistd.h> // For access, getcwd
#elif defined(ZFS_WINDOWS)
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h> // For everything
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX
#endif

#if !defined(Z_FS_NO_PATH) || !defined(Z_FS_NO_DIRECTORY)

// If Z_FS_NO_PATH is defined, but not Z_FS_NO_DIRECTORY,
// include path functions anyway, but as internal (static) functions.
#if defined(Z_FS_NO_PATH) && !defined(Z_FS_NO_DIRECTORY)
#define ZFSPATHDEF static
#else
#define ZFSPATHDEF ZFSDEF
#endif

#if defined(ZFS_POSIX)
static const char ZFS__DIR_SEP = '/';
#elif defined(ZFS_WINDOWS)
static const char ZFS__DIR_SEP = '\\';
#endif

static inline zfs_bool zfs__is_dir_sep(const char c)
{
	return (c == '/' || c == '\\');
}

static inline zfs_ll zfs__find_last_dir_sep(const char *path, zfs_ll len)
{
	zfs_ll index = len;
	while (index-- > 0 && !zfs__is_dir_sep(path[index]));
	return index;
}

static inline zfs_ll zfs__find_last_char(const char *path, zfs_ll len, char c)
{
	zfs_ll index = len;
	while (index-- > 0 && path[index] != c);
	return index;
}

ZFSPATHDEF void zfs_path_join(char *result, zfs_ll result_size, const char *left, const char *right)
{
	zfs_ll left_len = strlen(left);
	zfs_ll right_len = strlen(right);

	if (result != left)
	{
		if (left_len >= result_size)
			left_len = result_size - 1;
		memcpy(result, left, left_len);
	}

	if (left_len + 1 < result_size && left_len > 0 && !zfs__is_dir_sep(result[left_len - 1]))
		result[left_len++] = ZFS__DIR_SEP;

	if (left_len > 0 && right_len > 0 && zfs__is_dir_sep(right[0]))
	{
		++right;
		--right_len;
	}

	if (left_len + right_len >= result_size)
		right_len = result_size - left_len - 1;
	memcpy(result + left_len, right, right_len);

	result[left_len + right_len] = '\0';
}

ZFSPATHDEF void zfs_path_extension(char *result, zfs_ll result_size, const char *path)
{
	zfs_ll len = strlen(path);
	zfs_ll dir_sep_index = zfs__find_last_dir_sep(path, len) + 1;
	zfs_ll ext_index = zfs__find_last_char(path, len, '.');
	if (ext_index < dir_sep_index)
		ext_index = len;

	len -= ext_index;
	if (len >= result_size)
		len = result_size - 1;
	memcpy(result, path + ext_index, len);
	result[len] = '\0';
}

ZFSPATHDEF void zfs_path_set_extension(char *result, zfs_ll result_size, const char *path, const char *new_extension)
{
	zfs_ll len = strlen(path);
	zfs_ll dir_sep_index = zfs__find_last_dir_sep(path, len) + 1;
	zfs_ll ext_index = zfs__find_last_char(path, len, '.');
	if (ext_index < dir_sep_index)
		ext_index = len;

	if (result != path)
	{
		if (ext_index >= result_size)
			ext_index = result_size - 1;
		memcpy(result, path, ext_index);
	}

	zfs_ll ext_len = strlen(new_extension);
	if (ext_index + ext_len >= result_size)
		ext_len = result_size - ext_index - 1;
	memcpy(result + ext_index, new_extension, ext_len);

	result[ext_index + ext_len] = '\0';
}

ZFSPATHDEF void zfs_path_basename(char *result, zfs_ll result_size, const char *path)
{
	zfs_ll len = strlen(path);
	zfs_ll offset = zfs__find_last_dir_sep(path, len) + 1;
	len -= offset;
	if (len >= result_size)
		len = result_size - 1;
	memcpy(result, path + offset, len);
	result[len] = '\0';
}

ZFSPATHDEF void zfs_path_basename_without_extension(char *result, zfs_ll result_size, const char *path)
{
	zfs_ll len = strlen(path);
	zfs_ll dir_sep_index = zfs__find_last_dir_sep(path, len) + 1;
	zfs_ll ext_index = zfs__find_last_char(path, len, '.');
	if (ext_index < dir_sep_index)
		ext_index = len;

	len = ext_index - dir_sep_index;
	if (len >= result_size)
		len = result_size - 1;
	memcpy(result, path + dir_sep_index, len);
	result[len] = '\0';
}

ZFSPATHDEF void zfs_path_directory(char *result, zfs_ll result_size, const char *path)
{
	zfs_ll len = strlen(path);
	zfs_ll dir_sep_index = zfs__find_last_dir_sep(path, len);
	len = dir_sep_index + 1;
	if (len >= result_size)
		len = result_size - 1;
	memcpy(result, path, len);
	result[len] = '\0';
}

ZFSPATHDEF void zfs_path_normalize_inplace(char *path)
{
	while (*path)
	{
		if (zfs__is_dir_sep(*path))
			(*path) = ZFS__DIR_SEP;
		++path;
	}
}

ZFSPATHDEF void zfs_path_normalize(char *result, zfs_ll result_size, const char *path)
{
	zfs_ll len = strlen(path);
	if (len >= result_size)
		len = result_size - 1;
	memcpy(result, path, len);
	result[len] = '\0';

	zfs_path_normalize_inplace(result);
}

ZFSPATHDEF zfs_bool zfs_path_working_directory(char *result, zfs_ll result_size)
{
#if defined(ZFS_POSIX)
	return (getcwd(result, result_size) != NULL);
#elif defined(ZFS_WINDOWS)
	return (GetCurrentDirectoryA((DWORD)result_size, result) != 0);
#endif
}

ZFSPATHDEF void zfs_path_full(char *result, zfs_ll result_size, const char *path)
{
	if (zfs__is_dir_sep(path[0]))
	{
		zfs_path_join(result, result_size, "", path);
		return;
	}

	if (!zfs_path_working_directory(result, result_size))
	{
		if (result_size > 0)
			result[0] = '\0';
	}
	zfs_path_join(result, result_size, result, path);
}
#endif // Z_FS_NO_PATH

#ifndef Z_FS_NO_FILE
ZFSDEF zfs_bool zfs_file_touch(const char *filename)
{
	if (zfs_file_exists(filename))
	{
#if defined(ZFS_POSIX)
		return (utimes(filename, NULL) == 0);
#elif defined(ZFS_WINDOWS)
		SYSTEMTIME st;
		GetSystemTime(&st);
		FILETIME ft;
		if (!SystemTimeToFileTime(&st, &ft))
			return ZFS_FALSE;

		HANDLE handle = CreateFileA(filename, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (handle == INVALID_HANDLE_VALUE)
			return ZFS_FALSE;
		zfs_bool result = (SetFileTime(handle, NULL, &ft, &ft) != 0);
		CloseHandle(handle);
		return result;
#endif
	}
	else
	{
		FILE *file = fopen(filename, "wb");
		if (file == NULL)
			return ZFS_FALSE;
		fclose(file);
		return ZFS_TRUE;
	}
}

ZFSDEF zfs_bool zfs_file_exists(const char *filename)
{
#if defined(ZFS_POSIX)
	return (access(filename, F_OK) == 0);
#elif defined(ZFS_WINDOWS)
	HANDLE handle = CreateFileA(filename, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE)
		return ZFS_FALSE;
	CloseHandle(handle);
	return ZFS_TRUE;
#endif
}

ZFSDEF zfs_bool zfs_file_rename(const char *old_filename, const char *new_filename)
{
	return (rename(old_filename, new_filename) == 0);
}

ZFSDEF zfs_bool zfs_file_copy(const char *source_filename, const char *destination_filename)
{
	FILE *source_file = fopen(source_filename, "rb");
	if (!source_file)
		return ZFS_FALSE;

	FILE *destination_file = fopen(destination_filename, "wb");
	if (!destination_file)
	{
		fclose(source_file);
		return ZFS_FALSE;
	}

	char buffer[32768]; // 32k

	zfs_ll n;
	while ((n = fread(buffer, 1, sizeof(buffer), source_file)) > 0)
	{
		if (fwrite(buffer, 1, n, destination_file) != n)
		{
			fclose(source_file);
			fclose(destination_file);
			return ZFS_FALSE;
		}
	}

	fclose(source_file);
	fclose(destination_file);
	return ZFS_TRUE;
}

ZFSDEF zfs_bool zfs_file_delete(const char *filename)
{
	return (remove(filename) == 0);
}
#endif // Z_FS_NO_FILE

#ifndef Z_FS_NO_DIRECTORY
static inline zfs_bool zfs__skip_directory(ZFSDir *context)
{
	const char *name =
#if defined(ZFS_POSIX)
		((struct dirent*)context->data)->d_name;
#elif defined(ZFS_WINDOWS)
		((LPWIN32_FIND_DATAA)context->data)->cFileName;
#endif
	return (strcmp(name, ".") == 0 || strcmp(name, "..") == 0);
}

ZFSDEF zfs_bool zfs_directory_begin(ZFSDir *context, const char *path)
{
#if defined(ZFS_POSIX)
	context->handle = opendir(path);
	context->data = NULL;
	if (!context->handle)
		return ZFS_FALSE;

	if (!zfs_directory_next(context))
	{
		zfs_directory_end(context);
		return ZFS_FALSE;
	}

	return ZFS_TRUE;
#elif defined(ZFS_WINDOWS)
	char new_path[MAX_PATH];
	zfs_path_join(new_path, sizeof(new_path), path, "\\*");

	context->handle = FindFirstFileA(new_path, (LPWIN32_FIND_DATAA)&context->data);
	if (context->handle == INVALID_HANDLE_VALUE)
		return ZFS_FALSE;

	while (zfs__skip_directory(context))
	{
		if (!FindNextFileA(context->handle, (LPWIN32_FIND_DATAA)&context->data))
		{
			zfs_directory_end(context);
			return ZFS_FALSE;
		}
	}

	return ZFS_TRUE;
#endif
}

ZFSDEF zfs_bool zfs_directory_next(ZFSDir *context)
{
#if defined(ZFS_POSIX)
	do
	{
		context->data = readdir((DIR*)context->handle);
		if (!context->data)
			return ZFS_FALSE;
	} while (zfs__skip_directory(context));
	return ZFS_TRUE;
#elif defined(ZFS_WINDOWS)
	do
	{
		if (!FindNextFileA(context->handle, (LPWIN32_FIND_DATAA)&context->data))
			return ZFS_FALSE;
	} while (zfs__skip_directory(context));
	return ZFS_TRUE;
#endif
}

ZFSDEF void zfs_directory_end(ZFSDir *context)
{
#if defined(ZFS_POSIX)
	closedir((DIR*)context->handle);
	context->handle = NULL;
	context->data = NULL;
#elif defined(ZFS_WINDOWS)
	FindClose(context->handle);
	context->handle = NULL;
	memset(&context->data, 0, sizeof(context->data));
#endif
}

ZFSDEF void zfs_directory_current_filename(ZFSDir *context, char *result, zfs_ll result_size)
{
#if defined(ZFS_POSIX)
	if (context->data)
	{
		const char *name = ((struct dirent*)context->data)->d_name;
		zfs_ll name_size = strlen(name);
		if (name_size >= result_size)
			name_size = result_size - 1;
		memcpy(result, name, name_size);
		result[name_size] = '\0';
	}
	else if (result_size > 0)
	{
		result[0] = '\0';
	}
#elif defined(ZFS_WINDOWS)
	const char *name = ((LPWIN32_FIND_DATAA)context->data)->cFileName;
	zfs_ll name_size = strlen(name);
	if (name_size >= result_size)
		name_size = result_size - 1;
	memcpy(result, name, name_size);
	result[name_size] = '\0';
#endif
}

ZFSDEF zfs_bool zfs_directory_is_directory(ZFSDir *context)
{
#if defined(ZFS_POSIX)
	#ifdef _DIRENT_HAVE_D_TYPE
	return (S_ISDIR(DTTOIF(((struct dirent*)context->data)->d_type)) != 0);
	#else
	//TODO(johannes): Does not work yet, we need to convert d_name to full path first
	struct stat buf;
	if (stat(((struct dirent*)context->data)->d_name, &buf) != 0)
		return ZFS_FALSE;
	return (S_ISDIR(buf.st_mode) != 0);
	#endif
#elif defined(ZFS_WINDOWS)
	return (((LPWIN32_FIND_DATAA)context->data)->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#endif
}
#endif // Z_FS_NO_DIRECTORY

#endif // Z_FS_IMPLEMENTATION

#undef _CRT_SECURE_NO_WARNINGS
#undef ZFS_POSIX
#undef ZFS_WINDOWS
