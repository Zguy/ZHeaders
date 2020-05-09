#include "picotest_logger.h"

#define Z_IO_IMPLEMENTATION
#include "z_io.h"

#include <stdio.h>

const char TEST_TEXT[] = "This is a test\n";

static void write_test(ZIOHandle *handle)
{
	zio_ll size = sizeof(TEST_TEXT) - 1;
	PICOTEST_ASSERT(zio_write(handle, TEST_TEXT, size) == size);
}

static void write_test_should_fail(ZIOHandle *handle)
{
	PICOTEST_ASSERT(zio_write(handle, TEST_TEXT, sizeof(TEST_TEXT) - 1) == ZIO_ERROR);
}

static void read_test(ZIOHandle *handle)
{
	char read_text[sizeof(TEST_TEXT)];
	zio_ll size = sizeof(read_text) - 1;
	PICOTEST_ASSERT(zio_read(handle, read_text, size) == size);
	read_text[sizeof(read_text) - 1] = '\0';

	PICOTEST_ASSERT(strcmp(TEST_TEXT, read_text) == 0);
}

static void read_test_should_fail(ZIOHandle *handle)
{
	char read_text[sizeof(TEST_TEXT)];
	memset(read_text, 0, sizeof(read_text));
	PICOTEST_ASSERT(zio_read(handle, read_text, sizeof(read_text) - 1) == ZIO_ERROR);
	read_text[sizeof(read_text) - 1] = '\0';

	PICOTEST_ASSERT(strcmp(TEST_TEXT, read_text) != 0);
}

PICOTEST_CASE(file)
{
	ZIOHandle handle;
	PICOTEST_ASSERT(zio_open_file(&handle, "test.txt", ZIOM_WRITE | ZIOM_READ) == ZIO_OK);
	write_test(&handle);
	PICOTEST_ASSERT(zio_seek(&handle, 0, ZIO_SEEK_SET) == ZIO_OK);
	read_test(&handle);
	PICOTEST_ASSERT(zio_close(&handle) == ZIO_OK);

	PICOTEST_ASSERT(zio_open_file(&handle, "test.txt", ZIOM_WRITE) == ZIO_OK);
	write_test(&handle);
	PICOTEST_ASSERT(zio_seek(&handle, 0, ZIO_SEEK_SET) == ZIO_OK);
	read_test_should_fail(&handle);
	PICOTEST_ASSERT(zio_close(&handle) == ZIO_OK);

	PICOTEST_ASSERT(zio_open_file(&handle, "test.txt", ZIOM_READ) == ZIO_OK);
	write_test_should_fail(&handle);
	PICOTEST_ASSERT(zio_seek(&handle, 0, ZIO_SEEK_SET) == ZIO_OK);
	read_test(&handle);
	PICOTEST_ASSERT(zio_close(&handle) == ZIO_OK);

	remove("test.txt");
}

PICOTEST_CASE(memory)
{
	char mem[100];
	ZIOHandle handle;
	PICOTEST_ASSERT(zio_open_memory(&handle, mem, sizeof(mem)) == ZIO_OK);
	write_test(&handle);
	PICOTEST_ASSERT(zio_seek(&handle, 0, ZIO_SEEK_SET) == ZIO_OK);
	read_test(&handle);
	PICOTEST_ASSERT(zio_close(&handle) == ZIO_OK);
}

PICOTEST_CASE(const_memory)
{
	char mem[100];
	strcpy(mem, TEST_TEXT);

	ZIOHandle handle;
	zio_open_const_memory(&handle, mem, sizeof(mem));
	write_test_should_fail(&handle);
	PICOTEST_ASSERT(zio_seek(&handle, 0, ZIO_SEEK_SET) == ZIO_OK);
	read_test(&handle);
	PICOTEST_ASSERT(zio_close(&handle) == ZIO_OK);
}

int main(void)
{
	int fails = 0;
	fails += file(NULL);
	fails += memory(NULL);
	fails += const_memory(NULL);
	return fails;
}
