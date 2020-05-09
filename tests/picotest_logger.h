#include "picotest.h"

#include <stdio.h>

PicoTestFailureLoggerProc logFailure;
#undef PICOTEST_FAILURE_LOGGER
#define PICOTEST_FAILURE_LOGGER logFailure

/* Test failure logger function. */
void logFailure(const char *file, int line, const char *type, const char *test, const char *msg, va_list args)
{
	/* Error type. */
	printf("[%s] ", type);

	/* Location in source code. */
	printf("%s(%d) : ", file, line);

	/* Failed expression. */
	printf("%s", test);

	/* Optional message. */
	if (msg) {
		printf(" | ");
		vprintf(msg, args);
	}

	printf("\n");
}
