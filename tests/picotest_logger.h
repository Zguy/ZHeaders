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

/* Hooks */
PicoTestCaseEnterProc logEnter;
PicoTestCaseLeaveProc logLeave;
#undef PICOTEST_CASE_ENTER
#undef PICOTEST_CASE_LEAVE
#define PICOTEST_CASE_ENTER logEnter
#define PICOTEST_CASE_LEAVE logLeave

int g_level = 0;
void indent(int level) {
	while (level--) printf("  ");
}
void logEnter(const char *name) {
	indent(g_level++);
	printf("begin %s\n", name);
}
void logLeave(const char *name, int fail) {
	g_level--;
	if (!fail)
	{
		indent(g_level);
		printf("end %s\n", name);
	}
}
