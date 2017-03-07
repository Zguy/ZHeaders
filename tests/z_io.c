#define Z_IO_IMPLEMENTATION
#include "z_io.h"

#include <assert.h>
#include <stdio.h>

const char TEST_TEXT[] = "This is a test\n";

void write_test(ZIOHandle *handle)
{
	assert(zio_write(handle, TEST_TEXT, sizeof(TEST_TEXT) - 1, 1) == 1);
}

void write_test_should_fail(ZIOHandle *handle)
{
	assert(zio_write(handle, TEST_TEXT, sizeof(TEST_TEXT) - 1, 1) == 0);
}

void read_test(ZIOHandle *handle)
{
	char read_text[sizeof(TEST_TEXT)];
	assert(zio_read(handle, read_text, sizeof(read_text) - 1, 1) == 1);
	read_text[sizeof(read_text) - 1] = '\0';

	assert(strcmp(TEST_TEXT, read_text) == 0);
}

void read_test_should_fail(ZIOHandle *handle)
{
	char read_text[sizeof(TEST_TEXT)];
	memset(read_text, 0, sizeof(read_text));
	assert(zio_read(handle, read_text, sizeof(read_text) - 1, 1) == 0);
	read_text[sizeof(read_text) - 1] = '\0';

	assert(strcmp(TEST_TEXT, read_text) != 0);
}

int main()
{
	{
		ZIOHandle handle;
		assert(zio_open_file(&handle, "test.txt", ZIOM_WRITE | ZIOM_READ) == 0);
		write_test(&handle);
		assert(zio_seek(&handle, 0, ZIO_SEEK_SET) == 0);
		read_test(&handle);
		assert(zio_close(&handle) == 0);

		assert(zio_open_file(&handle, "test.txt", ZIOM_WRITE) == 0);
		write_test(&handle);
		assert(zio_seek(&handle, 0, ZIO_SEEK_SET) == 0);
		read_test_should_fail(&handle);
		assert(zio_close(&handle) == 0);

		assert(zio_open_file(&handle, "test.txt", ZIOM_READ) == 0);
		write_test_should_fail(&handle);
		assert(zio_seek(&handle, 0, ZIO_SEEK_SET) == 0);
		read_test(&handle);
		assert(zio_close(&handle) == 0);

		remove("test.txt");
	}

	{
		char mem[100];
		ZIOHandle handle;
		assert(zio_open_memory(&handle, mem, sizeof(mem)) == 0);
		write_test(&handle);
		assert(zio_seek(&handle, 0, ZIO_SEEK_SET) == 0);
		read_test(&handle);
		assert(zio_close(&handle) == 0);
	}

	{
		char mem[100];
		strcpy(mem, TEST_TEXT);

		ZIOHandle handle;
		zio_open_const_memory(&handle, mem, sizeof(mem));
		write_test_should_fail(&handle);
		assert(zio_seek(&handle, 0, ZIO_SEEK_SET) == 0);
		read_test(&handle);
		assert(zio_close(&handle) == 0);
	}
	return 0;
}
