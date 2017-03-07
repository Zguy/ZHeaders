#define Z_IO_IMPLEMENTATION
#include "z_io.h"

#include <assert.h>
#include <stdio.h>

const char TEST_TEXT[] = "This is a test\n";

void write_test(ZIOHandle *handle)
{
	zio_ll size = sizeof(TEST_TEXT) - 1;
	assert(zio_write(handle, TEST_TEXT, size) == size);
}

void write_test_should_fail(ZIOHandle *handle)
{
	assert(zio_write(handle, TEST_TEXT, sizeof(TEST_TEXT) - 1) == ZIO_ERROR);
}

void read_test(ZIOHandle *handle)
{
	char read_text[sizeof(TEST_TEXT)];
	zio_ll size = sizeof(read_text) - 1;
	assert(zio_read(handle, read_text, size) == size);
	read_text[sizeof(read_text) - 1] = '\0';

	assert(strcmp(TEST_TEXT, read_text) == 0);
}

void read_test_should_fail(ZIOHandle *handle)
{
	char read_text[sizeof(TEST_TEXT)];
	memset(read_text, 0, sizeof(read_text));
	assert(zio_read(handle, read_text, sizeof(read_text) - 1) == ZIO_ERROR);
	read_text[sizeof(read_text) - 1] = '\0';

	assert(strcmp(TEST_TEXT, read_text) != 0);
}

int main()
{
	{
		ZIOHandle handle;
		assert(zio_open_file(&handle, "test.txt", ZIOM_WRITE | ZIOM_READ) == ZIO_OK);
		write_test(&handle);
		assert(zio_seek(&handle, 0, ZIO_SEEK_SET) == ZIO_OK);
		read_test(&handle);
		assert(zio_close(&handle) == ZIO_OK);

		assert(zio_open_file(&handle, "test.txt", ZIOM_WRITE) == ZIO_OK);
		write_test(&handle);
		assert(zio_seek(&handle, 0, ZIO_SEEK_SET) == ZIO_OK);
		read_test_should_fail(&handle);
		assert(zio_close(&handle) == ZIO_OK);

		assert(zio_open_file(&handle, "test.txt", ZIOM_READ) == ZIO_OK);
		write_test_should_fail(&handle);
		assert(zio_seek(&handle, 0, ZIO_SEEK_SET) == ZIO_OK);
		read_test(&handle);
		assert(zio_close(&handle) == ZIO_OK);

		remove("test.txt");
	}

	{
		char mem[100];
		ZIOHandle handle;
		assert(zio_open_memory(&handle, mem, sizeof(mem)) == ZIO_OK);
		write_test(&handle);
		assert(zio_seek(&handle, 0, ZIO_SEEK_SET) == ZIO_OK);
		read_test(&handle);
		assert(zio_close(&handle) == ZIO_OK);
	}

	{
		char mem[100];
		strcpy(mem, TEST_TEXT);

		ZIOHandle handle;
		zio_open_const_memory(&handle, mem, sizeof(mem));
		write_test_should_fail(&handle);
		assert(zio_seek(&handle, 0, ZIO_SEEK_SET) == ZIO_OK);
		read_test(&handle);
		assert(zio_close(&handle) == ZIO_OK);
	}
	return 0;
}
