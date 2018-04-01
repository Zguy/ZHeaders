#define Z_FS_IMPLEMENTATION
//#define Z_FS_NO_PATH
//#define Z_FS_NO_FILE
//#define Z_FS_NO_DIRECTORY
#include "z_filesystem.h"

#include <assert.h>

//TODO(johannes): Test robustness when buffer is too small

static void assert_strcmp(const char *str1, const char *str2)
{
	assert(strcmp(str1, str2) == 0);
}

#ifndef Z_FS_NO_PATH
static void assert_normalized_strcmp(const char *str1, const char *str2)
{
	char str1_norm[50];
	char str2_norm[50];
	zfs_path_normalize(str1_norm, sizeof(str1_norm), str1);
	zfs_path_normalize(str2_norm, sizeof(str2_norm), str2);
	assert(strcmp(str1_norm, str2_norm) == 0);
}
#endif

int main()
{
	char buffer[50];

#ifndef Z_FS_NO_PATH
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

	assert(zfs_path_working_directory(buffer, sizeof(buffer)) == ZFS_TRUE);
	zfs_path_full(buffer, sizeof(buffer), "file.txt");
	zfs_path_full(buffer, sizeof(buffer), "/usr/bin/file.txt");
#endif

#ifndef Z_FS_NO_FILE
	assert(zfs_file_touch("test.txt") == ZFS_TRUE);
	assert(zfs_file_exists("test.txt") == ZFS_TRUE);
	assert(zfs_file_rename("test.txt", "test2.txt") == ZFS_TRUE);
	assert(zfs_file_touch("test2.txt") == ZFS_TRUE);
	assert(zfs_file_exists("test2.txt") == ZFS_TRUE);
	assert(zfs_file_copy("test2.txt", "test.txt") == ZFS_TRUE);
	assert(zfs_file_delete("test.txt") == ZFS_TRUE);
	assert(zfs_file_exists("test.txt") == ZFS_FALSE);
	assert(zfs_file_delete("test2.txt") == ZFS_TRUE);
	assert(zfs_file_exists("test2.txt") == ZFS_FALSE);
#endif

#ifndef Z_FS_NO_DIRECTORY
	ZFSDir dir;
	if (zfs_directory_begin(&dir, "../tests"))
	{
		while (zfs_directory_next(&dir))
		{
			zfs_directory_current_filename(&dir, buffer, sizeof(buffer));
			ZFSBool is_dir = zfs_directory_is_directory(&dir);
			printf("%i\n", is_dir);
		}
		zfs_directory_end(&dir);
	}
#endif
	return 0;
}
