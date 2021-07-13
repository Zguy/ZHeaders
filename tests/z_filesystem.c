#include "picotest_logger.h"

#define Z_FS_IMPLEMENTATION
//#define Z_FS_NO_PATH
//#define Z_FS_NO_FILE
//#define Z_FS_NO_DIRECTORY
//#define Z_FS_ALWAYS_FORWARD_SLASH
#include "z_filesystem.h"

static void assert_strcmp(const char *str1, const char *str2)
{
	PICOTEST_ASSERT(strcmp(str1, str2) == 0, "\"%s\" and \"%s\" do not match", str1, str2);
}

#ifndef Z_FS_NO_PATH
static void assert_normalized_strcmp(const char *str1, const char *str2)
{
	char str1_norm[50];
	char str2_norm[50];
	zfs_path_normalize(str1_norm, sizeof(str1_norm), str1);
	zfs_path_normalize(str2_norm, sizeof(str2_norm), str2);
	PICOTEST_ASSERT(strcmp(str1_norm, str2_norm) == 0, "\"%s\" and \"%s\" do not match", str1_norm, str2_norm);
}

PICOTEST_CASE(path)
{
	char buffer[50];

	zfs_path_join(buffer, sizeof(buffer), "/usr/bin", "file");
	assert_normalized_strcmp(buffer, "/usr/bin/file");
	zfs_path_join(buffer, sizeof(buffer), "/usr/bin/", "file");
	assert_normalized_strcmp(buffer, "/usr/bin/file");
	zfs_path_join(buffer, sizeof(buffer), "/usr/bin/", "/file");
	assert_normalized_strcmp(buffer, "/usr/bin/file");
	zfs_path_join(buffer, sizeof(buffer), "/usr/bin/", "");
	assert_normalized_strcmp(buffer, "/usr/bin/");
	zfs_path_join(buffer, sizeof(buffer), "/usr/bin", "");
	assert_normalized_strcmp(buffer, "/usr/bin/");
	zfs_path_join(buffer, sizeof(buffer), "", "/file");
	assert_normalized_strcmp(buffer, "/file");

	zfs_path_extension(buffer, sizeof(buffer), "/usr/bin/file.txt");
	assert_strcmp(buffer, ".txt");
	zfs_path_extension(buffer, sizeof(buffer), "file.txt");
	assert_strcmp(buffer, ".txt");
	zfs_path_extension(buffer, sizeof(buffer), "/file");
	assert_strcmp(buffer, "");
	zfs_path_extension(buffer, sizeof(buffer), "file");
	assert_strcmp(buffer, "");
	zfs_path_extension(buffer, sizeof(buffer), "/weird.path/to/file.txt");
	assert_strcmp(buffer, ".txt");
	zfs_path_extension(buffer, sizeof(buffer), "/wierd.path/to/file");
	assert_strcmp(buffer, "");
	zfs_path_extension(buffer, sizeof(buffer), "file.with.multiple.extensions.txt");
	assert_strcmp(buffer, ".txt");

	zfs_path_set_extension(buffer, sizeof(buffer), "/usr/bin/file.txt", ".bmp");
	assert_strcmp(buffer, "/usr/bin/file.bmp");
	zfs_path_set_extension(buffer, sizeof(buffer), "file.txt", ".bmp");
	assert_strcmp(buffer, "file.bmp");
	zfs_path_set_extension(buffer, sizeof(buffer), "/file", ".bmp");
	assert_strcmp(buffer, "/file.bmp");
	zfs_path_set_extension(buffer, sizeof(buffer), "file", ".bmp");
	assert_strcmp(buffer, "file.bmp");
	zfs_path_set_extension(buffer, sizeof(buffer), "/weird.path/to/file.txt", ".bmp");
	assert_strcmp(buffer, "/weird.path/to/file.bmp");
	zfs_path_set_extension(buffer, sizeof(buffer), "/wierd.path/to/file", ".bmp");
	assert_strcmp(buffer, "/wierd.path/to/file.bmp");
	zfs_path_set_extension(buffer, sizeof(buffer), "file.with.multiple.extensions.txt", ".bmp");
	assert_strcmp(buffer, "file.with.multiple.extensions.bmp");

	zfs_path_basename(buffer, sizeof(buffer), "/usr/bin/file");
	assert_strcmp(buffer, "file");
	zfs_path_basename(buffer, sizeof(buffer), "file.txt");
	assert_strcmp(buffer, "file.txt");
	zfs_path_basename(buffer, sizeof(buffer), "/file.txt");
	assert_strcmp(buffer, "file.txt");

	zfs_path_basename_without_extension(buffer, sizeof(buffer), "/usr/bin/file");
	assert_strcmp(buffer, "file");
	zfs_path_basename_without_extension(buffer, sizeof(buffer), "file.txt");
	assert_strcmp(buffer, "file");
	zfs_path_basename_without_extension(buffer, sizeof(buffer), "/file.txt");
	assert_strcmp(buffer, "file");
	zfs_path_basename_without_extension(buffer, sizeof(buffer), "/file");
	assert_strcmp(buffer, "file");
	zfs_path_basename_without_extension(buffer, sizeof(buffer), "file");
	assert_strcmp(buffer, "file");
	zfs_path_basename_without_extension(buffer, sizeof(buffer), "/weird.path/to/file");
	assert_strcmp(buffer, "file");

	zfs_path_directory(buffer, sizeof(buffer), "/usr/bin/file");
	assert_normalized_strcmp(buffer, "/usr/bin/");
	zfs_path_directory(buffer, sizeof(buffer), "file.txt");
	assert_normalized_strcmp(buffer, "");
	zfs_path_directory(buffer, sizeof(buffer), "/file.txt");
	assert_normalized_strcmp(buffer, "/");

	zfs_path_normalize(buffer, sizeof(buffer), "/mixed\\separators/here\\");
	assert_normalized_strcmp(buffer, "/mixed/separators/here/");

	strcpy(buffer, "/mixed\\separators/here\\");
	zfs_path_normalize_inplace(buffer);
	assert_normalized_strcmp(buffer, "/mixed/separators/here/");

	PICOTEST_ASSERT(zfs_path_working_directory(buffer, sizeof(buffer)) == ZFS_TRUE);
	zfs_path_full(buffer, sizeof(buffer), "file.txt");
	zfs_path_full(buffer, sizeof(buffer), "/usr/bin/file.txt");
}

PICOTEST_CASE(path_tiny_buffer)
{
	char buffer[5];

	zfs_path_join(buffer, sizeof(buffer), "/usr/bin", "file");
	assert_normalized_strcmp(buffer, "/usr");
	zfs_path_join(buffer, sizeof(buffer), "/usr/bin/", "file");
	assert_normalized_strcmp(buffer, "/usr");
	zfs_path_join(buffer, sizeof(buffer), "/usr/bin/", "/file");
	assert_normalized_strcmp(buffer, "/usr");
	zfs_path_join(buffer, sizeof(buffer), "/usr/bin/", "");
	assert_normalized_strcmp(buffer, "/usr");
	zfs_path_join(buffer, sizeof(buffer), "/usr/bin", "");
	assert_normalized_strcmp(buffer, "/usr");
	zfs_path_join(buffer, sizeof(buffer), "", "/file");
	assert_normalized_strcmp(buffer, "/fil");

	zfs_path_extension(buffer, sizeof(buffer), "/usr/bin/file.txt");
	assert_strcmp(buffer, ".txt");
	zfs_path_extension(buffer, sizeof(buffer), "file.txt");
	assert_strcmp(buffer, ".txt");
	zfs_path_extension(buffer, sizeof(buffer), "/file");
	assert_strcmp(buffer, "");
	zfs_path_extension(buffer, sizeof(buffer), "file");
	assert_strcmp(buffer, "");
	zfs_path_extension(buffer, sizeof(buffer), "/weird.path/to/file.txt");
	assert_strcmp(buffer, ".txt");
	zfs_path_extension(buffer, sizeof(buffer), "/wierd.path/to/file");
	assert_strcmp(buffer, "");
	zfs_path_extension(buffer, sizeof(buffer), "file.with.multiple.extensions.txt");
	assert_strcmp(buffer, ".txt");

	zfs_path_set_extension(buffer, sizeof(buffer), "/usr/bin/file.txt", ".bmp");
	assert_strcmp(buffer, "/usr");
	zfs_path_set_extension(buffer, sizeof(buffer), "file.txt", ".bmp");
	assert_strcmp(buffer, "file");
	zfs_path_set_extension(buffer, sizeof(buffer), "/file", ".bmp");
	assert_strcmp(buffer, "/fil");
	zfs_path_set_extension(buffer, sizeof(buffer), "file", ".bmp");
	assert_strcmp(buffer, "file");
	zfs_path_set_extension(buffer, sizeof(buffer), "/weird.path/to/file.txt", ".bmp");
	assert_strcmp(buffer, "/wei");
	zfs_path_set_extension(buffer, sizeof(buffer), "/wierd.path/to/file", ".bmp");
	assert_strcmp(buffer, "/wie");
	zfs_path_set_extension(buffer, sizeof(buffer), "file.with.multiple.extensions.txt", ".bmp");
	assert_strcmp(buffer, "file");

	zfs_path_basename(buffer, sizeof(buffer), "/usr/bin/file");
	assert_strcmp(buffer, "file");
	zfs_path_basename(buffer, sizeof(buffer), "file.txt");
	assert_strcmp(buffer, "file");
	zfs_path_basename(buffer, sizeof(buffer), "/file.txt");
	assert_strcmp(buffer, "file");

	zfs_path_basename_without_extension(buffer, sizeof(buffer), "/usr/bin/file");
	assert_strcmp(buffer, "file");
	zfs_path_basename_without_extension(buffer, sizeof(buffer), "file.txt");
	assert_strcmp(buffer, "file");
	zfs_path_basename_without_extension(buffer, sizeof(buffer), "/file.txt");
	assert_strcmp(buffer, "file");
	zfs_path_basename_without_extension(buffer, sizeof(buffer), "/file");
	assert_strcmp(buffer, "file");
	zfs_path_basename_without_extension(buffer, sizeof(buffer), "file");
	assert_strcmp(buffer, "file");
	zfs_path_basename_without_extension(buffer, sizeof(buffer), "/weird.path/to/file");
	assert_strcmp(buffer, "file");

	zfs_path_directory(buffer, sizeof(buffer), "/usr/bin/file");
	assert_normalized_strcmp(buffer, "/usr");
	zfs_path_directory(buffer, sizeof(buffer), "file.txt");
	assert_normalized_strcmp(buffer, "");
	zfs_path_directory(buffer, sizeof(buffer), "/file.txt");
	assert_normalized_strcmp(buffer, "/");

	zfs_path_normalize(buffer, sizeof(buffer), "/m\\s");
	assert_normalized_strcmp(buffer, "/m/s");

	strcpy(buffer, "/m\\s");
	zfs_path_normalize_inplace(buffer);
	assert_normalized_strcmp(buffer, "/m/s");
}

PICOTEST_CASE(path_buffer_left)
{
	char buffer[50];

	strcpy(buffer, "/root");
	zfs_path_join(buffer, sizeof(buffer), buffer, "another/path");
	assert_normalized_strcmp(buffer, "/root/another/path");

	zfs_path_set_extension(buffer, sizeof(buffer), buffer, ".bmp");
	assert_normalized_strcmp(buffer, "/root/another/path.bmp");
}
#endif

#ifndef Z_FS_NO_FILE
PICOTEST_CASE(file)
{
	PICOTEST_ASSERT(zfs_file_touch("test.txt") == ZFS_TRUE);
	PICOTEST_ASSERT(zfs_file_exists("test.txt") == ZFS_TRUE);
	PICOTEST_ASSERT(zfs_file_rename("test.txt", "test2.txt") == ZFS_TRUE);
	PICOTEST_ASSERT(zfs_file_touch("test2.txt") == ZFS_TRUE);
	PICOTEST_ASSERT(zfs_file_exists("test2.txt") == ZFS_TRUE);
	PICOTEST_ASSERT(zfs_file_copy("test2.txt", "test.txt") == ZFS_TRUE);
	PICOTEST_ASSERT(zfs_file_delete("test.txt") == ZFS_TRUE);
	PICOTEST_ASSERT(zfs_file_exists("test.txt") == ZFS_FALSE);
	PICOTEST_ASSERT(zfs_file_delete("test2.txt") == ZFS_TRUE);
	PICOTEST_ASSERT(zfs_file_exists("test2.txt") == ZFS_FALSE);
}
#endif

#ifndef Z_FS_NO_DIRECTORY
PICOTEST_CASE(directory)
{
	ZFSDir dir;
	if (zfs_directory_begin(&dir, "tests"))
	{
		do
		{
			const char *filename = zfs_directory_current_filename(&dir);
			zfs_bool is_dir = zfs_directory_is_directory(&dir);
			printf("%s = %i\n", filename, is_dir);
		} while (zfs_directory_next(&dir));
		zfs_directory_end(&dir);
	}
}
#endif

int main(void)
{
	int fails = 0;
#ifndef Z_FS_NO_PATH
	fails += path(NULL);
	fails += path_tiny_buffer(NULL);
	fails += path_buffer_left(NULL);
#endif
#ifndef Z_FS_NO_FILE
	fails += file(NULL);
#endif
#ifndef Z_FS_NO_DIRECTORY
	fails += directory(NULL);
#endif
	return fails;
}
