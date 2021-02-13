/**
 * @file picotest.h
 *
 * This file defines a minimalist unit testing framework for C programs.
 *
 * The assertion mechanism relies on `setjmp()` / `longjmp()`. While these
 * functions are discouraged for production code, their usage is acceptable in
 * the context of unit testing: in our case, `longjmp()` is only called when an
 * assertion fails, a situation where the actual process state is no longer
 * reliable anyway. Moreover, they constitute the only standard exception
 * handling mechanism for plain C code.
 *
 * @par License
 *
 * PicoTest is released under the terms of the The 3-Clause BSD License:
 *
 * https://opensource.org/licenses/BSD-3-Clause
 *
 * Copyright (c) 2018 Frederic Bonnet
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _PICOTEST
#define _PICOTEST

#include <setjmp.h>
#include <stdarg.h>
#include <string.h>

#if defined(_MSC_VER)
/** \internal
 * MSVC is buggy wrt. (__VA_ARGS__) syntax. The workaround involves the use of a
 * dummy macro before the parentheses. See the following for an alternate
 * solution:
 *      http://www.gamedev.net/community/forums/topic.asp?topic_id=567686
 */
#define _PICOTEST_PARENS
#endif /* defined(_MSC_VER) */

/*!
 * \defgroup public_interface Public interface
 * \{
 */

/*!
 * \name Version
 * PicoTest follows the Semantic Versioning Specification (SemVer) 2.0.0:
 *
 * https://semver.org/spec/v2.0.0.html
 * \{
 */

#define PICOTEST_VERSION "1.4.1"
#define PICOTEST_VERSION_MAJOR 1
#define PICOTEST_VERSION_MINOR 4
#define PICOTEST_VERSION_PATCH 1

/*! \} End of Version */

/*!
 * \name Test Functions
 * \{
 */

/**
 * Signature of test functions.
 *
 * Both @ref test_suites and @ref test_cases follow this signature.
 *
 * @param cond  Test filtering condition, or **NULL**. In the former case,
 *              passed to the active test filter before running the test.
 *
 * @return Number of failed tests.
 *
 * @see PICOTEST_SUITE
 * @see PICOTEST_CASE
 * @see PICOTEST_FILTER
 */
typedef int(PicoTestProc)(const char *cond);

/**
 * Test metadata.
 */
typedef struct PicoTestMetadata {
    const char *name;         /*!< Test name. */
    const char *file;         /*!< Test file location. */
    int line;                 /*!< Test line location. */
    PicoTestProc *const test; /*!< Test function. */
    int nbSubtests;           /*!< Number of subtests. */
    const struct PicoTestMetadata *
        *subtests; /*!< Subtests (NULL-terminated array for test suites, NULL
                      pointer for test cases). */
} PicoTestMetadata;

/**
 * Declare an extern test for metadata access.
 *
 * @param _testName     Test name.
 *
 * @see PICOTEST_METADATA
 */
#define PICOTEST_EXTERN(_testName)                                             \
    extern PicoTestProc _testName;                                             \
    extern PicoTestMetadata _PICOTEST_METADATA(_testName);

/**
 * Get test metadata.
 *
 * @note Tests in other modules need to be declared first with PICOTEST_EXTERN.
 *
 * @param _testName     Test name.
 *
 * @see PicoTestMetadata
 * @see PICOTEST_EXTERN
 */
#define PICOTEST_METADATA(_testName) &_PICOTEST_METADATA(_testName)

/*! \cond IGNORE */
#define _PICOTEST_METADATA(_testName) _testName##_metadata
#define _PICOTEST_TEST_DECLARE(_testName, _nbSubtests, _subtests)              \
    PicoTestProc _testName;                                                    \
    PicoTestMetadata _PICOTEST_METADATA(_testName) = {                         \
        _PICOTEST_STRINGIZE(_testName),                                        \
        __FILE__,                                                              \
        __LINE__,                                                              \
        _testName,                                                             \
        _nbSubtests,                                                           \
        (const struct PicoTestMetadata **)_subtests};
/*! \endcond */

/*! \} End of Test Functions */

/*!
 * \name Test Filters
 *
 * PicoTest provides a way for client code to select tests to be run using
 * custom filter functions.
 * \{
 */

/**
 * Result of test filter functions.
 *
 * @par Examples
 *      @example_file{filter.c}
 *      @example_file{tags.c}
 *
 * @see PicoTestFilterProc
 */
typedef enum PicoTestFilterResult {
    /** Test does not match the condition, skip this test and all its
     *  subtests. */
    PICOTEST_FILTER_SKIP = 0,

    /** Test matches the condition, run this test and all its subtests. */
    PICOTEST_FILTER_PASS = 1,

    /** Test does not match the condition, skip this test but filter its
     *  subtests. */
    PICOTEST_FILTER_SKIP_PROPAGATE = 2,

    /** Test matches the condition, run this test but filter its subtests. */
    PICOTEST_FILTER_PASS_PROPAGATE = 3,
} PicoTestFilterResult;

/**
 * Signature of test filter functions.
 *
 * A test called with a non- **NULL** condition must match this condition to be
 * run. The test filter is set using the @ref PICOTEST_FILTER macro.
 *
 * @param test      Test function to filter.
 * @param testName  Name of test to filter.
 * @param cond      Test filtering condition.
 *
 * @return a @ref PicoTestFilterResult value
 *
 * @par Usage
 *      @snippet filter.c   PICOTEST_FILTER example
 *
 * @par Examples
 *      @example_file{filter.c}
 *      @example_file{tags.c}
 *
 * @see PICOTEST_SUITE
 * @see PICOTEST_CASE
 * @see PICOTEST_FILTER
 * @see PicoTestFilterResult
 */
typedef PicoTestFilterResult(PicoTestFilterProc)(PicoTestProc *test,
                                                 const char *testName,
                                                 const char *cond);

/** \internal
 * Implementation of default test filter function.
 *
 * Does a simple string equality test between **testName** and **cond**, and
 * propagates to subtests if it doesn't match.
 *
 * @see PicoTestFailureLoggerProc
 * @see PICOTEST_FAILURE_LOGGER
 * @see PICOTEST_FAILURE_LOGGER_DEFAULT
 */
static PicoTestFilterResult _picoTest_filterByName(PicoTestProc *test,
                                                   const char *testName,
                                                   const char *cond) {
    return (strcmp(testName, cond) == 0 ? PICOTEST_FILTER_PASS
                                        : PICOTEST_FILTER_SKIP_PROPAGATE);
}

/**
 * Default test filter function.
 *
 * Does a simple string equality test between **testName** and **cond**, and
 * propagates to subtests if it doesn't match.
 *
 * @see PicoTestFilterProc
 * @see PICOTEST_FILTER
 */
#define PICOTEST_FILTER_DEFAULT _picoTest_filterByName

/**
 * Define the test filter function.
 *
 * Called before calling a test with a non- **NULL** condition.
 *
 * The default filter does a simple string equality test between its
 * **testName** and **cond** arguments, and propagates to subtests if it
 * doesn't match. Redefine this macro to use a custom filter function, which
 * must follow the @ref PicoTestFilterProc signature.
 *
 * @note Custom functions only apply to the tests defined after the macro
 * redefinition. As macros can be redefined several times, this means that
 * different functions may apply for the same source.
 *
 * @par Usage
 *      @snippet filter.c   PICOTEST_FILTER example
 *
 * @par Examples
 *      @example_file{filter.c}
 *      @example_file{tags.c}
 *
 * @see PicoTestFilterProc
 * @see PICOTEST_FILTER_DEFAULT
 */
#define PICOTEST_FILTER PICOTEST_FILTER_DEFAULT

/*! \} End of Test Filters */

/*!
 * \name Test hierarchy traversal
 *
 * Tests can form hierarchies of test suites and test cases. PicoTest provides
 * two ways to traverse such hierarchies with a simple visitor pattern. This can
 * be used for e.g. test list discovery in build systems.
 * \{
 */

/**
 * Function signature of test traversal proc.
 *
 * @param name  Name of traversed test.
 * @param nb    Number of subtests (zero for simple test cases, at least one
 *              for test suites).
 *
 * @par Usage
 *      @snippet traverse.c     PicoTestTraverseProc example
 *
 * @par Examples
 *      @example_file{traverse.c}
 *
 * @see PICOTEST_TRAVERSE
 */
typedef void(PicoTestTraverseProc)(const char *name, int nb);

/**
 * Traverse a test hierarchy depth-first.
 *
 * This feature covers simple use cases such as getting the flat list of all
 * test names. For more advanced usage, see #PICOTEST_VISIT.
 *
 * @param _testName     Name of the traversed test.
 * @param _proc         Test traversal proc. Must follow the @ref
 *                      PicoTestTraverseProc signature.
 *
 * @par Examples
 *      @example_file{traverse.c}
 *
 * @see PicoTestTraverseProc
 * @see PICOTEST_VISIT
 */
#define PICOTEST_TRAVERSE(_testName, _proc)                                    \
    _picoTest_traverse(PICOTEST_METADATA(_testName), _proc)

/** \internal
 * Perform test traversal.
 *
 * @param metadata      Metadata of test to traverse.
 * @param proc          Test traversal proc.
 *
 * @see PicoTestTraverseProc
 * @see PicoTestMetadata
 * @see PICOTEST_TRAVERSE
 */
static void _picoTest_traverse(const PicoTestMetadata *metadata,
                               PicoTestTraverseProc *proc) {
    const PicoTestMetadata **subtest = metadata->subtests;
    proc(metadata->name, metadata->nbSubtests);
    if (metadata->nbSubtests) {
        for (; *subtest; subtest++) {
            _picoTest_traverse(*subtest, proc);
        }
    }
}

/**
 * Test visit step.
 *
 * @see PicoTestVisitProc
 * @see PICOTEST_VISIT
 */
typedef enum PicoTestVisitStep {
    /** Enter the test. */
    PICOTEST_VISIT_ENTER = 0,

    /** Leave the test. */
    PICOTEST_VISIT_LEAVE = 1,
} PicoTestVisitStep;

/**
 * Function signature of test visit proc.
 *
 * Proc is called once for each value of #PicoTestVisitStep.
 *
 * @param metadata      Metadata of the visited test.
 * @param step          Visit step.
 *
 * @see PICOTEST_VISIT
 * @see PicoTestVisitStep
 */
typedef void(PicoTestVisitProc)(const PicoTestMetadata *metadata,
                                PicoTestVisitStep step);

/**
 * Visit a test hierarchy depth-first.
 *
 * This feature covers more advanced use cases than #PICOTEST_TRAVERSE, such as
 * exporting the test hierarchy as a structured format such as XML or JSON,
 * or accessing test metadata.
 *
 * @param _testName     Name of the visited test.
 * @param _proc         Test visit proc. Must follow the @ref
 *                      PicoTestVisitProc signature.
 *
 * @see PicoTestVisitProc
 * @see PICOTEST_TRAVERSE
 */
#define PICOTEST_VISIT(_testName, _proc)                                       \
    _picoTest_visit(PICOTEST_METADATA(_testName), _proc)

/** \internal
 * Perform test visit.
 *
 * @param metadata      Metadata of test to visit.
 * @param proc          Test visit proc.
 *
 * @see PicoTestVisitProc
 * @see PicoTestMetadata
 * @see PICOTEST_VISIT
 */
static void _picoTest_visit(const PicoTestMetadata *metadata,
                            PicoTestVisitProc *proc) {
    const PicoTestMetadata **subtest = metadata->subtests;
    proc(metadata, PICOTEST_VISIT_ENTER);
    if (metadata->nbSubtests) {
        for (; *subtest; subtest++) {
            _picoTest_visit(*subtest, proc);
        }
    }
    proc(metadata, PICOTEST_VISIT_LEAVE);
}

/*! \} End of Test Traversal */

/*!
 * \name Logging
 *
 * PicoTest provides a way for client code to intercept test failure events.
 * This can be used for e.g. logging purpose or reporting.
 * \{
 */

/**
 * Function signature of test failure log handlers.
 *
 * @param file  File name where the test was defined.
 * @param line  Location of test in file.
 * @param type  Type of test that failed (e.g. ``"ASSERT"``).
 * @param test  Tested expression.
 * @param msg   Optional message format string, or **NULL**.
 * @param args  Optional message string parameter list, or **NULL**.
 *
 * @note **msg** and **args** are suitable arguments to **vprintf()**.
 *
 * @par Usage
 *      @snippet logger.c   PICOTEST_FAILURE_LOGGER example
 *
 * @par Examples
 *      @example_file{logger.c}
 *
 * @see PICOTEST_FAILURE_LOGGER
 */
typedef void(PicoTestFailureLoggerProc)(const char *file, int line,
                                        const char *type, const char *test,
                                        const char *msg, va_list args);

/** \internal
 * Implementation of default test failure log handler. Does nothing.
 *
 * @see PicoTestFailureLoggerProc
 * @see PICOTEST_FAILURE_LOGGER
 * @see PICOTEST_FAILURE_LOGGER_DEFAULT
 */
static void _picoTest_logFailure(const char *file, int line, const char *type,
                                 const char *test, const char *msg,
                                 va_list args) {}

/**
 * Default test failure log handler. Does nothing.
 *
 * @see PicoTestFailureLoggerProc
 * @see PICOTEST_FAILURE_LOGGER
 */
#define PICOTEST_FAILURE_LOGGER_DEFAULT _picoTest_logFailure

/**
 * Define the test failure log handler. Called when a test fails.
 *
 * The default handler does nothing. Redefine this macro to use a custom
 * handler, which must follow the @ref PicoTestFailureLoggerProc signature.
 *
 * @note Custom functions only apply to the tests defined after the macro
 * redefinition. As macros can be redefined several times, this means that
 * different functions may apply for the same source.
 *
 * @par Usage
 *      @snippet logger.c   PICOTEST_FAILURE_LOGGER example
 *
 * @par Examples
 *      @example_file{logger.c}
 *
 * @see PicoTestFailureLoggerProc
 * @see PICOTEST_FAILURE_LOGGER_DEFAULT
 */
#define PICOTEST_FAILURE_LOGGER PICOTEST_FAILURE_LOGGER_DEFAULT

/*! \} End of Logging */

/*!
 * \defgroup test_cases Test Cases
 *
 * Test cases are the most elementary test functions. They are defined as simple
 * functions blocks with assertions that checks the validity of the outcome.
 * \{
 */

/*!
 * \name Test Case Definitions
 * \{
 */

/**
 * Test case declaration.
 *
 * This macro defines a @ref PicoTestProc of the given name that can be called
 * directly.
 *
 * @param _testName     Name of the test case.
 * @param _fixtureName  (optional) Name of the test fixture used by the test.
 * @param _context      (optional) Fixture context structure defined using
 *                      PICOTEST_FIXTURE_CONTEXT(_fixtureName).
 *
 * @return Number of failed tests.
 *
 * @par Usage
 *      @snippet mainSuite.inc  PICOTEST_CASE examples
 *
 * @par Examples
 *      @example_file{mainSuite.inc}
 *
 * @see PicoTestProc
 * @see PICOTEST_FIXTURE_CONTEXT
 */
#if defined(_PICOTEST_PARENS)
#define PICOTEST_CASE(...)                                                     \
    _PICOTEST_CONCATENATE(_PICOTEST_CASE_, _PICOTEST_ARGCOUNT(__VA_ARGS__))    \
    _PICOTEST_PARENS(__VA_ARGS__)
#else
#define PICOTEST_CASE(...)                                                     \
    _PICOTEST_CONCATENATE(_PICOTEST_CASE_, _PICOTEST_ARGCOUNT(__VA_ARGS__))    \
    (__VA_ARGS__)
#endif /* defined(_PICOTEST_PARENS) */

/*! \cond IGNORE */
#define _PICOTEST_CASE_DECLARE(_testName)                                      \
    _PICOTEST_TEST_DECLARE(_testName, 0, NULL);                                \
    static int _testName##_testCaseRunner(void);                               \
    int _testName(const char *cond) {                                          \
        int fail = 0;                                                          \
        PicoTestFilterResult filterResult =                                    \
            (cond == NULL)                                                     \
                ? PICOTEST_FILTER_PASS                                         \
                : PICOTEST_FILTER(_testName, _PICOTEST_STRINGIZE(_testName),   \
                                  cond);                                       \
        switch (filterResult) {                                                \
        case PICOTEST_FILTER_PASS:                                             \
        case PICOTEST_FILTER_PASS_PROPAGATE:                                   \
            fail += _testName##_testCaseRunner();                              \
            break;                                                             \
        }                                                                      \
        return fail;                                                           \
    }

#define _PICOTEST_CASE_RUNNER_BEGIN(_testName)                                 \
    static int _testName##_testCaseRunner() {                                  \
        int abort;                                                             \
        jmp_buf failureEnv;                                                    \
        jmp_buf *oldEnv = _picoTest_failureEnv;                                \
        int fail, oldFail = _picoTest_fail;                                    \
        _picoTest_failureEnv = &failureEnv;                                    \
        _picoTest_fail = 0;                                                    \
        PICOTEST_CASE_ENTER(_PICOTEST_STRINGIZE(_testName));                   \
        abort = setjmp(failureEnv);

#define _PICOTEST_CASE_RUNNER_END(_testName)                                   \
    fail = _picoTest_fail;                                                     \
    PICOTEST_CASE_LEAVE(_PICOTEST_STRINGIZE(_testName), fail);                 \
    _picoTest_failureEnv = oldEnv;                                             \
    _picoTest_fail = oldFail;                                                  \
    return fail;                                                               \
    }

#define _PICOTEST_CASE_1(_testName)                                            \
    _PICOTEST_CASE_DECLARE(_testName)                                          \
    static void _testName##_testCase(void);                                    \
    _PICOTEST_CASE_RUNNER_BEGIN(_testName)                                     \
    if (!abort) {                                                              \
        _testName##_testCase();                                                \
    }                                                                          \
    _PICOTEST_CASE_RUNNER_END(_testName)                                       \
    static void _testName##_testCase(void)

#define _PICOTEST_CASE_2(_testName, _fixtureName)                              \
    _PICOTEST_CASE_DECLARE(_testName)                                          \
    static void _testName##_testCase(void);                                    \
    _PICOTEST_CASE_RUNNER_BEGIN(_testName)                                     \
    if (!abort) {                                                              \
        _PICOTEST_FIXTURE_CALL_SETUP(_fixtureName, _testName, NULL);           \
        _testName##_testCase();                                                \
    }                                                                          \
    _PICOTEST_FIXTURE_CALL_TEARDOWN(_fixtureName, _testName, NULL,             \
                                    _picoTest_fail);                           \
    _PICOTEST_CASE_RUNNER_END(_testName)                                       \
    static void _testName##_testCase()

#define _PICOTEST_CASE_3(_testName, _fixtureName, _context)                    \
    _PICOTEST_CASE_DECLARE(_testName)                                          \
    static void _testName##_testCase(struct _fixtureName##_Context *);         \
    _PICOTEST_CASE_RUNNER_BEGIN(_testName) {                                   \
        struct _fixtureName##_Context context;                                 \
        if (!abort) {                                                          \
            _PICOTEST_FIXTURE_CALL_SETUP(_fixtureName, _testName, &context);   \
            _testName##_testCase(&context);                                    \
        }                                                                      \
        _PICOTEST_FIXTURE_CALL_TEARDOWN(_fixtureName, _testName, &context,     \
                                        _picoTest_fail);                       \
    }                                                                          \
    _PICOTEST_CASE_RUNNER_END(_testName)                                       \
    static void _testName##_testCase(struct _fixtureName##_Context *_context)
/*! \endcond */

/*! \} End of Test Case Definitions */

/*!
 * \name Test Case Hooks
 *
 * PicoTest provides a way for client code to intercept test case events. This
 * can be used for e.g. logging purpose or reporting.
 * \{
 */

/**
 * Function signature of test case enter hooks.
 *
 * Called before running the test case.
 *
 * @param testName      Test case name.
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_CASE_ENTER example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PICOTEST_CASE_ENTER
 */
typedef void(PicoTestCaseEnterProc)(const char *testName);

/**
 * Default test case enter hook. Does nothing.
 *
 * @see PicoTestCaseEnterProc
 * @see PICOTEST_CASE_ENTER
 */
#define PICOTEST_CASE_ENTER_DEFAULT(testName)

/**
 * Define the test case enter hook.
 *
 * The default hook does nothing. Redefine this macro to use a custom hook,
 * which must follow the @ref PicoTestCaseEnterProc signature.
 *
 * @note Custom functions only apply to the tests defined after the macro
 * redefinition. As macros can be redefined several times, this means that
 * different functions may apply for the same source.
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_CASE_ENTER example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PicoTestCaseEnterProc
 * @see PICOTEST_CASE_ENTER_DEFAULT
 * @see PICOTEST_CASE_LEAVE
 */
#define PICOTEST_CASE_ENTER PICOTEST_CASE_ENTER_DEFAULT

/**
 * Function signature of test case leave hooks.
 *
 * Called after running the test case.
 *
 * @param testName      Test case name.
 * @param fail          Failed tests (including its subtests if any).
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_CASE_LEAVE example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PICOTEST_CASE_LEAVE
 */
typedef void(PicoTestCaseLeaveProc)(const char *testName, int fail);

/**
 * Default test case enter hook. Does nothing.
 *
 * @see PicoTestCaseLeaveProc
 * @see PICOTEST_CASE_LEAVE
 */
#define PICOTEST_CASE_LEAVE_DEFAULT(testName, fail)

/**
 * Define the test case leave hook.
 *
 * The default hook does nothing. Redefine this macro to use a custom hook,
 * which must follow the @ref PicoTestCaseLeaveProc signature.
 *
 * @note Custom functions only apply to the tests defined after the macro
 * redefinition. As macros can be redefined several times, this means that
 * different functions may apply for the same source.
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_CASE_LEAVE example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PicoTestCaseLeaveProc
 * @see PICOTEST_CASE_LEAVE_DEFAULT
 * @see PICOTEST_CASE_ENTER
 */
#define PICOTEST_CASE_LEAVE PICOTEST_CASE_LEAVE_DEFAULT

/*! \} End of Test Case Hooks */

/*! \} End of Test Cases */

/*!
 * \defgroup assertions Assertions
 *
 * Assertions are the basic building blocks of test cases.
 * \{
 */

/*!
 * \name Assertion Definitions
 * \{
 */

/**
 * Hard assertion. Logs an error if the given value is false, then stops the
 * test with PICOTEST_ABORT().
 *
 * PICOTEST_FAILURE_LOGGER() is called with the **type** argument set to
 * ``"ASSERT"``.
 *
 * @param x     Value to test. Evaluated once, so it can be an expression with
 *              side effects.
 * @param msg   (optional) Message format string.
 * @param ...   (optional) Message string arguments.
 *
 * @note **msg** and following arguments arguments are suitable arguments to
 * **printf()**.
 *
 * @par Examples
 *      @example_file{mainSuite.inc}
 *
 * @see PICOTEST_FAILURE_LOGGER
 * @see PICOTEST_ABORT
 * @see PICOTEST_VERIFY
 */
#define PICOTEST_ASSERT(x, /* msg, */...)                                      \
    { _PICOTEST_ASSERT(x, #x, ##__VA_ARGS__); }

/*! \cond IGNORE */
#define _PICOTEST_ASSERT(x, ...)                                               \
    PICOTEST_ASSERT_BEFORE("ASSERT", #x);                                      \
    {                                                                          \
        int _PICOTEST_FAIL = !(x);                                             \
        PICOTEST_ASSERT_AFTER("ASSERT", #x, _PICOTEST_FAIL);                   \
        if (_PICOTEST_FAIL) {                                                  \
            PICOTEST_FAILURE("ASSERT", ##__VA_ARGS__);                         \
            PICOTEST_ABORT();                                                  \
        }                                                                      \
    }
/*! \endcond */

/**
 * Soft assertion. Logs an error if the given value is false, but let the test
 * continue.
 *
 * PICOTEST_FAILURE_LOGGER() is called with the **type** argument set to
 * ``"VERIFY"``.
 *
 * @param x     Value to test. Evaluated once, so it can be an expression with
 *              side effects.
 * @param msg   (optional) Message format string.
 * @param ...   (optional) Message string arguments.
 *
 * @note **msg** and following arguments arguments are suitable arguments to
 * **printf()**.
 *
 * @par Examples
 *      @example_file{mainSuite.inc}
 *
 * @see PICOTEST_FAILURE_LOGGER
 * @see PICOTEST_ASSERT
 */
#define PICOTEST_VERIFY(x, /* msg, */...)                                      \
    { _PICOTEST_VERIFY(x, #x, ##__VA_ARGS__); }

/*! \cond IGNORE */
#define _PICOTEST_VERIFY(x, ...)                                               \
    PICOTEST_ASSERT_BEFORE("VERIFY", #x);                                      \
    {                                                                          \
        int _PICOTEST_FAIL = !(x);                                             \
        PICOTEST_ASSERT_AFTER("VERIFY", #x, _PICOTEST_FAIL);                   \
        if (_PICOTEST_FAIL) {                                                  \
            PICOTEST_FAILURE("VERIFY", ##__VA_ARGS__);                         \
        }                                                                      \
    }
/*! \endcond */

/**
 * Generic failure.
 *
 * PICOTEST_FAILURE_LOGGER() is called with the provided **type**, **test** and
 * **msg** arguments.
 *
 * This can be used to implement custom testing logic.
 *
 * @param type  Type of test that failed (e.g. "ASSERT").
 * @param test  Failed test.
 * @param msg   (optional) Message format string.
 * @param ...   (optional) Message string arguments.
 */
#define PICOTEST_FAILURE(type, test, /* msg, */...)                            \
    _PICOTEST_FAILURE(type, test, ##__VA_ARGS__)

/*! \cond IGNORE */
#define _PICOTEST_FAILURE(type, ...)                                           \
    _picoTest_fail++;                                                          \
    _picoTest_assertFailed(PICOTEST_FAILURE_LOGGER, __FILE__, __LINE__, type,  \
                           _PICOTEST_ARGCOUNT(__VA_ARGS__), __VA_ARGS__);      \
/*! \endcond */

/** \internal
 * Internal failure counter.
 *
 * @see PICOTEST_FAILURE
 */
static int _picoTest_fail = 0;

/** \internal
 * Tag used by **setjmp()** and **longjmp()** to jump out of failed tests.
 *
 * @see PICOTEST_ABORT
 * @see PICOTEST_CASE
 */
static jmp_buf *_picoTest_failureEnv = NULL;

/**
 * Abort a test case.
 *
 * This can be used to implement custom testing logic.
 *
 * @see PICOTEST_CASE
 */
#define PICOTEST_ABORT() longjmp(*_picoTest_failureEnv, 1)

/** \internal
 * Called when an assertion fails.
 *
 * @param proc  Test failure log handler.
 * @param file  File name where the test was defined.
 * @param line  Location of test in file.
 * @param type  Type of test that failed (e.g. "ASSERT").
 * @param count Number of arguments after **test**.
 * @param test  Tested expression.
 * @param ...   Optional message format string and parameters.
 *
 * @see PICOTEST_ASSERT
 * @see PICOTEST_VERIFY
 */
static void _picoTest_assertFailed(PicoTestFailureLoggerProc *proc,
                                   const char *file, int line, const char *type,
                                   int count, const char *test, ...) {
    if (count > 1) {
        /* Extra args after **test** */
        va_list args;
        const char *msg;
        va_start(args, test);
        msg = va_arg(args, const char *);
        proc(file, line, type, test, msg, args);
    } else {
        proc(file, line, type, test, NULL, NULL);
    }
}

/*! \} End of Assertion Definitions */

/*!
 * \name Assertion Hooks
 *
 * PicoTest provides a way for client code to intercept assertions events. This
 * can be used for e.g. logging purpose or reporting.
 * \{
 */

/**
 * Function signature of assert before hooks.
 *
 * Called before running an assertion.
 *
 * @param type  Type of test (e.g. "ASSERT").
 * @param test  Test.
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_ASSERT_BEFORE example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PICOTEST_ASSERT_BEFORE
 */
typedef void(PicoTestAssertBeforeProc)(const char *type, const char *test);

/**
 * Default assert before hook. Does nothing.
 *
 * @see PicoTestAssertBeforeProc
 * @see PICOTEST_ASSERT_BEFORE
 */
#define PICOTEST_ASSERT_BEFORE_DEFAULT(type, test)

/**
 * Define the assert before hook.
 *
 * The default hook does nothing. Redefine this macro to use a custom hook,
 * which must follow the @ref PicoTestAssertBeforeProc signature.
 *
 * @note Custom functions only apply to the tests defined after the macro
 * redefinition. As macros can be redefined several times, this means that
 * different functions may apply for the same source.
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_ASSERT_BEFORE example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PicoTestAssertBeforeProc
 * @see PICOTEST_ASSERT_BEFORE_DEFAULT
 * @see PICOTEST_ASSERT_AFTER
 */
#define PICOTEST_ASSERT_BEFORE PICOTEST_ASSERT_BEFORE_DEFAULT

/**
 * Function signature of assert after hooks.
 *
 * Called after running an assertion.
 *
 * @param type  Type of test (e.g. "ASSERT").
 * @param test  Test.
 * @param fail  Test result: zero for success, non-zero for failure.
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_ASSERT_AFTER example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PICOTEST_ASSERT_AFTER
 */
typedef void(PicoTestAssertAfterProc)(const char *type, const char *test,
                                      int fail);

/**
 * Default assert after hook. Does nothing.
 *
 * @see PicoTestAssertAfterProc
 * @see PICOTEST_ASSERT_AFTER
 */
#define PICOTEST_ASSERT_AFTER_DEFAULT(type, test, fail)

/**
 * Define the assert after hook.
 *
 * The default hook does nothing. Redefine this macro to use a custom hook,
 * which must follow the @ref PicoTestAssertAfterProc signature.
 *
 * @note Custom functions only apply to the tests defined after the macro
 * redefinition. As macros can be redefined several times, this means that
 * different functions may apply for the same source.
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_ASSERT_AFTER example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PicoTestAssertAfterProc
 * @see PICOTEST_ASSERT_AFTER_DEFAULT
 * @see PICOTEST_ASSERT_BEFORE
 */
#define PICOTEST_ASSERT_AFTER PICOTEST_ASSERT_AFTER_DEFAULT

/*! \} End of Assertion Hooks */

/*! \} End of Assertions */

/*!
 * \defgroup fixtures Test Fixtures
 *
 * Test fixtures are used to establish the context needed to run test cases.
 *
 * A test fixture can be used by several, possibly unrelated test cases.
 * \{
 */

/*!
 * \name Test Fixture Definitions
 * \{
 */

/**
 * Test fixture context declaration.
 *
 * Fixtures can optionally define a context structure that is passed to its
 * setup and teardown functions.
 *
 * @param _fixtureName      Name of the fixture.
 *
 * @par Usage
 *      @snippet mainSuite.inc  PICOTEST_FIXTURE_CONTEXT example
 *
 * @par Examples
 *      @example_file{mainSuite.inc}
 *      @example_file{fixtures.c}
 *
 * @see PICOTEST_FIXTURE_SETUP
 * @see PICOTEST_FIXTURE_TEARDOWN
 * @see PICOTEST_CASE
 */
#define PICOTEST_FIXTURE_CONTEXT(_fixtureName) struct _fixtureName##_Context

/**
 * Test fixture initialization.
 *
 * @param _fixtureName  Name of the fixture.
 * @param _context      (optional) Fixture context structure defined using
 *                      PICOTEST_FIXTURE_CONTEXT(_fixtureName).
 *
 * @par Usage
 * A simple fixture with no context:
 *      @snippet mainSuite.inc  Simple fixture
 *
 * A more complex example with a context structure:
 *      @snippet mainSuite.inc  Fixture with context
 *
 * Fixtures may define an optional context that test cases don't need, in this
 * case the context passed to the setup and teardown functions is **NULL**:
 *      @snippet mainSuite.inc  Fixture with optional context
 * Here is an example of such a test case:
 *      @snippet mainSuite.inc  PICOTEST_CASE with fixture and optional context
 *
 * @par Examples
 *      @example_file{mainSuite.inc}
 *      @example_file{fixtures.c}
 *
 * @see PICOTEST_FIXTURE_CONTEXT
 * @see PICOTEST_FIXTURE_TEARDOWN
 * @see PICOTEST_CASE
 */
#if defined(_PICOTEST_PARENS)
#define PICOTEST_FIXTURE_SETUP(...)                                            \
    _PICOTEST_CONCATENATE(_PICOTEST_FIXTURE_SETUP_,                            \
                          _PICOTEST_ARGCOUNT(__VA_ARGS__))                     \
    _PICOTEST_PARENS(__VA_ARGS__)
#else
#define PICOTEST_FIXTURE_SETUP(...)                                            \
    _PICOTEST_CONCATENATE(_PICOTEST_FIXTURE_SETUP_,                            \
                          _PICOTEST_ARGCOUNT(__VA_ARGS__))                     \
    (__VA_ARGS__)
#endif /* defined(_PICOTEST_PARENS) */

/*! \cond IGNORE */
#define _PICOTEST_FIXTURE_SETUP_1(_fixtureName)                                \
    static void _fixtureName##_setup(void *_fixtureName##_DUMMY)

#define _PICOTEST_FIXTURE_SETUP_2(_fixtureName, _context)                      \
    static void _fixtureName##_setup(struct _fixtureName##_Context *_context)

#define _PICOTEST_FIXTURE_CALL_SETUP(_fixtureName, _testName, context)         \
    PICOTEST_FIXTURE_BEFORE_SETUP(_PICOTEST_STRINGIZE(_fixtureName),           \
                                  _PICOTEST_STRINGIZE(_testName));             \
    _fixtureName##_setup(context);                                             \
    PICOTEST_FIXTURE_AFTER_SETUP(_PICOTEST_STRINGIZE(_fixtureName),            \
                                 _PICOTEST_STRINGIZE(_testName));
/*! \endcond */

/**
 * Test fixture cleanup.
 *
 * @param _fixtureName  Name of the fixture.
 * @param _context      (optional) Fixture context structure defined using
 *                      PICOTEST_FIXTURE_CONTEXT(_fixtureName).
 *
 * @par Usage
 * A simple fixture with no context:
 *      @snippet mainSuite.inc  Simple fixture
 *
 * A more complex example with a context structure:
 *      @snippet mainSuite.inc  Fixture with context
 *
 * Fixtures may define an optional context that test cases don't need, in this
 * case the context passed to the setup and teardown functions is **NULL**:
 *      @snippet mainSuite.inc  Fixture with optional context
 * Here is an example of such a test case:
 *      @snippet mainSuite.inc  PICOTEST_CASE with fixture and optional context
 *
 * @par Examples
 *      @example_file{mainSuite.inc}
 *      @example_file{fixtures.c}
 *
 * @see PICOTEST_FIXTURE_CONTEXT
 * @see PICOTEST_FIXTURE_SETUP
 * @see PICOTEST_CASE
 */
#if defined(_PICOTEST_PARENS)
#define PICOTEST_FIXTURE_TEARDOWN(...)                                         \
    _PICOTEST_CONCATENATE(_PICOTEST_FIXTURE_TEARDOWN_,                         \
                          _PICOTEST_ARGCOUNT(__VA_ARGS__))                     \
    _PICOTEST_PARENS(__VA_ARGS__)
#else
#define PICOTEST_FIXTURE_TEARDOWN(...)                                         \
    _PICOTEST_CONCATENATE(_PICOTEST_FIXTURE_TEARDOWN_,                         \
                          _PICOTEST_ARGCOUNT(__VA_ARGS__))                     \
    (__VA_ARGS__)
#endif /* defined(_PICOTEST_PARENS) */

/*! \cond IGNORE */
#define _PICOTEST_FIXTURE_TEARDOWN_1(_fixtureName)                             \
    static void _fixtureName##_teardown(int PICOTEST_FAIL,                     \
                                        void *_fixtureName##_DUMMY)

#define _PICOTEST_FIXTURE_TEARDOWN_2(_fixtureName, _context)                   \
    static void _fixtureName##_teardown(                                       \
        int PICOTEST_FAIL, struct _fixtureName##_Context *_context)

#define _PICOTEST_FIXTURE_CALL_TEARDOWN(_fixtureName, _testName, context,      \
                                        fail)                                  \
    PICOTEST_FIXTURE_BEFORE_TEARDOWN(_PICOTEST_STRINGIZE(_fixtureName),        \
                                     _PICOTEST_STRINGIZE(_testName), fail);    \
    _fixtureName##_teardown(fail, context);                                    \
    PICOTEST_FIXTURE_AFTER_TEARDOWN(_PICOTEST_STRINGIZE(_fixtureName),         \
                                    _PICOTEST_STRINGIZE(_testName), fail);     \
/*! \endcond */

/*! \} End of Test Fixture Definitions */

/*!
 * \name Test Fixture Hooks
 *
 * PicoTest provides a way for client code to intercept test fixture events.
 * This can be used for e.g. logging purpose or reporting.
 * \{
 */

/**
 * Function signature of test fixture before setup hooks.
 *
 * Called before running the test fixture setup.
 *
 * @param fixtureName   Test fixture name.
 * @param testName      Test case name.
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_FIXTURE_BEFORE_SETUP example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PICOTEST_FIXTURE_BEFORE_SETUP
 */
typedef void(PicoTestFixtureBeforeSetupProc)(const char *fixtureName,
                                             const char *testName);

/**
 * Default test fixture before setup hook. Does nothing.
 *
 * @see PicoTestFixtureBeforeSetupProc
 * @see PICOTEST_FIXTURE_BEFORE_SETUP
 */
#define PICOTEST_FIXTURE_BEFORE_SETUP_DEFAULT(fixtureName, testName)

/**
 * Define the test fixture before setup hook.
 *
 * The default hook does nothing. Redefine this macro to use a custom hook,
 * which must follow the @ref PicoTestFixtureBeforeSetupProc signature.
 *
 * @note Custom functions only apply to the tests defined after the macro
 * redefinition. As macros can be redefined several times, this means that
 * different functions may apply for the same source.
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_FIXTURE_BEFORE_SETUP example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PicoTestFixtureBeforeSetupProc
 * @see PICOTEST_FIXTURE_BEFORE_SETUP_DEFAULT
 * @see PICOTEST_FIXTURE_AFTER_SETUP
 */
#define PICOTEST_FIXTURE_BEFORE_SETUP PICOTEST_FIXTURE_BEFORE_SETUP_DEFAULT

/**
 * Function signature of test fixture after setup hooks.
 *
 * Called after running the test fixture setup.
 *
 * @param fixtureName   Test fixture name.
 * @param testName      Test case name.
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_FIXTURE_AFTER_SETUP example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PICOTEST_FIXTURE_AFTER_SETUP
 */
typedef void(PicoTestFixtureAfterSetupProc)(const char *fixtureName,
                                            const char *testName);

/**
 * Default test fixture after setup hook. Does nothing.
 *
 * @see PicoTestFixtureAfterSetupProc
 * @see PICOTEST_FIXTURE_AFTER_SETUP
 */
#define PICOTEST_FIXTURE_AFTER_SETUP_DEFAULT(fixtureName, testName)

/**
 * Define the test fixture after setup hook.
 *
 * The default hook does nothing. Redefine this macro to use a custom hook,
 * which must follow the @ref PicoTestFixtureAfterSetupProc signature.
 *
 * @note Custom functions only apply to the tests defined after the macro
 * redefinition. As macros can be redefined several times, this means that
 * different functions may apply for the same source.
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_FIXTURE_AFTER_SETUP example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PicoTestFixtureAfterSetupProc
 * @see PICOTEST_FIXTURE_AFTER_SETUP_DEFAULT
 * @see PICOTEST_FIXTURE_BEFORE_SETUP
 */
#define PICOTEST_FIXTURE_AFTER_SETUP PICOTEST_FIXTURE_AFTER_SETUP_DEFAULT

/**
 * Function signature of test fixture before teardown hooks.
 *
 * Called before running the test fixture teardown.
 *
 * @param fixtureName   Test fixture name.
 * @param testName      Test case name.
 * @param fail          Failed tests (including its subtests if any).
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_FIXTURE_BEFORE_TEARDOWN example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PICOTEST_FIXTURE_BEFORE_TEARDOWN
 */
typedef void(PicoTestFixtureBeforeTeardownProc)(const char *fixtureName,
                                                const char *testName, int fail);

/**
 * Default test fixture before teardown hook. Does nothing.
 *
 * @see PicoTestFixtureBeforeTeardownProc
 * @see PICOTEST_FIXTURE_BEFORE_TEARDOWN
 */
#define PICOTEST_FIXTURE_BEFORE_TEARDOWN_DEFAULT(fixtureName, testName, fail)

/**
 * Define the test fixture before teardown hook.
 *
 * The default hook does nothing. Redefine this macro to use a custom hook,
 * which must follow the @ref PicoTestFixtureBeforeTeardownProc signature.
 *
 * @note Custom functions only apply to the tests defined after the macro
 * redefinition. As macros can be redefined several times, this means that
 * different functions may apply for the same source.
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_FIXTURE_BEFORE_TEARDOWN example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PicoTestFixtureBeforeTeardownProc
 * @see PICOTEST_FIXTURE_BEFORE_TEARDOWN_DEFAULT
 * @see PICOTEST_FIXTURE_AFTER_TEARDOWN
 */
#define PICOTEST_FIXTURE_BEFORE_TEARDOWN                                       \
    PICOTEST_FIXTURE_BEFORE_TEARDOWN_DEFAULT

/**
 * Function signature of test fixture after teardown hooks.
 *
 * Called after running the test fixture teardown.
 *
 * @param fixtureName   Test fixture name.
 * @param testName      Test case name.
 * @param fail          Failed tests (including its subtests if any).
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_FIXTURE_AFTER_TEARDOWN example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PICOTEST_FIXTURE_AFTER_TEARDOWN
 */
typedef void(PicoTestFixtureAfterTeardownProc)(const char *fixtureName,
                                               const char *testName, int fail);

/**
 * Default test fixture after teardown hook. Does nothing.
 *
 * @see PicoTestFixtureAfterTeardownProc
 * @see PICOTEST_FIXTURE_AFTER_TEARDOWN
 */
#define PICOTEST_FIXTURE_AFTER_TEARDOWN_DEFAULT(fixtureName, testName, fail)

/**
 * Define the test fixture after teardown hook.
 *
 * The default hook does nothing. Redefine this macro to use a custom hook,
 * which must follow the @ref PicoTestFixtureAfterTeardownProc signature.
 *
 * @note Custom functions only apply to the tests defined after the macro
 * redefinition. As macros can be redefined several times, this means that
 * different functions may apply for the same source.
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_FIXTURE_AFTER_TEARDOWN example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PicoTestFixtureAfterTeardownProc
 * @see PICOTEST_FIXTURE_AFTER_TEARDOWN_DEFAULT
 * @see PICOTEST_FIXTURE_BEFORE_TEARDOWN
 */
#define PICOTEST_FIXTURE_AFTER_TEARDOWN PICOTEST_FIXTURE_AFTER_TEARDOWN_DEFAULT

/*! \} End of Test Fixture Hooks */

/*! \} End of Test Fixtures */

/*!
 * \defgroup test_suites Test Suites
 *
 * A test suite is a set of subtests in no special order. These subtests can
 * themselves be test suites or test cases.
 * \{
 */

/*!
 * \name Test Suite Definitions
 * \{
 */

/**
 * Test suite declaration.
 *
 * A test suite is a test function that is made of one or several subtests.
 *
 * This macro defines a @ref PicoTestProc of the given name that can be called
 * directly.
 *
 * @param _suiteName    Name of the test suite.
 * @param ...           Names of the subtests in the suite.
 *
 * @return Number of failed tests.
 *
 * @see PicoTestProc
 * @see PICOTEST_CASE
 *
 * @par Usage
 *      @snippet mainSuite.inc  PICOTEST_SUITE examples
 *
 * @par Examples
 *      @example_file{mainSuite.inc}
 */
#define PICOTEST_SUITE(_suiteName, ...)                                        \
    _PICOTEST_FOR_EACH(PICOTEST_EXTERN, __VA_ARGS__)                           \
    static PicoTestMetadata *_suiteName##_subtests[] = {_PICOTEST_FOR_EACH(    \
        _PICOTEST_SUITE_ENUMERATE_SUBTEST, __VA_ARGS__) NULL};                 \
    _PICOTEST_TEST_DECLARE(_suiteName, _PICOTEST_ARGCOUNT(__VA_ARGS__),        \
                           _suiteName##_subtests);                             \
    static int _suiteName##_testSuiteRunner(const char *cond) {                \
        const int nb = _PICOTEST_ARGCOUNT(__VA_ARGS__);                        \
        PicoTestMetadata **subtest = _suiteName##_subtests;                    \
        int fail = 0;                                                          \
        PICOTEST_SUITE_ENTER(_PICOTEST_STRINGIZE(_suiteName), nb);             \
        for (; *subtest; subtest++) {                                          \
            const int index = (int)(subtest - _suiteName##_subtests);          \
            int sfail = 0;                                                     \
            PICOTEST_SUITE_BEFORE_SUBTEST(_PICOTEST_STRINGIZE(_suiteName), nb, \
                                          fail, index, (*subtest)->name);      \
            sfail = (*subtest)->test(cond);                                    \
            fail += sfail;                                                     \
            PICOTEST_SUITE_AFTER_SUBTEST(_PICOTEST_STRINGIZE(_suiteName), nb,  \
                                         fail, index, (*subtest)->name,        \
                                         sfail);                               \
        }                                                                      \
        PICOTEST_SUITE_LEAVE(_PICOTEST_STRINGIZE(_suiteName), nb, fail);       \
        return fail;                                                           \
    }                                                                          \
    int _suiteName(const char *cond) {                                         \
        int fail = 0;                                                          \
        PicoTestFilterResult filterResult =                                    \
            (cond == NULL)                                                     \
                ? PICOTEST_FILTER_PASS                                         \
                : PICOTEST_FILTER(_suiteName, _PICOTEST_STRINGIZE(_suiteName), \
                                  cond);                                       \
        switch (filterResult) {                                                \
        case PICOTEST_FILTER_PASS:                                             \
            cond = NULL;                                                       \
        case PICOTEST_FILTER_PASS_PROPAGATE:                                   \
            fail += _suiteName##_testSuiteRunner(cond);                        \
            break;                                                             \
        case PICOTEST_FILTER_SKIP:                                             \
            break;                                                             \
        case PICOTEST_FILTER_SKIP_PROPAGATE: {                                 \
            PicoTestMetadata **subtest = _suiteName##_subtests;                \
            for (; *subtest; subtest++) {                                      \
                fail += (*subtest)->test(cond);                                \
            }                                                                  \
        } break;                                                               \
        }                                                                      \
        return fail;                                                           \
    }

/*! \cond IGNORE */
#define _PICOTEST_SUITE_ENUMERATE_SUBTEST(_testName)                           \
    PICOTEST_METADATA(_testName),
/*! \endcond */

/*! \} End of Test Suite Definitions */

/*!
 * \name Test Suite Hooks
 *
 * PicoTest provides a way for client code to intercept test execution events
 * on test suites and their subtests. This can be used for e.g. logging purpose
 * or reporting.
 * \{
 */

/**
 * Function signature of test suite enter hooks.
 *
 * Called before running the first subtest.
 *
 * @param suiteName     Test suite name.
 * @param nb            Number of subtests.
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_SUITE_ENTER example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PICOTEST_SUITE_ENTER
 */
typedef void(PicoTestSuiteEnterProc)(const char *suiteName, int nb);

/**
 * Default test suite enter hook. Does nothing.
 *
 * @see PicoTestSuiteEnterProc
 * @see PICOTEST_SUITE_ENTER
 */
#define PICOTEST_SUITE_ENTER_DEFAULT(suiteName, nb)

/**
 * Define the test suite enter hook.
 *
 * The default hook does nothing. Redefine this macro to use a custom hook,
 * which must follow the @ref PicoTestSuiteEnterProc signature.
 *
 * @note Custom functions only apply to the tests defined after the macro
 * redefinition. As macros can be redefined several times, this means that
 * different functions may apply for the same source.
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_SUITE_ENTER example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PicoTestSuiteEnterProc
 * @see PICOTEST_SUITE_ENTER_DEFAULT
 * @see PICOTEST_SUITE_LEAVE
 */
#define PICOTEST_SUITE_ENTER PICOTEST_SUITE_ENTER_DEFAULT

/**
 * Function signature of test suite leave hooks.
 *
 * @param suiteName     Test suite name.
 * @param nb            Number of subtests.
 * @param fail          Number of failed subtests (including the subtests'
 *                      subtests if any).
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_SUITE_LEAVE example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PICOTEST_SUITE_LEAVE
 */
typedef void(PicoTestSuiteLeaveProc)(const char *suiteName, int nb, int fail);

/**
 * Default test suite leave hook. Does nothing.
 *
 * @see PicoTestSuiteLeaveProc
 * @see PICOTEST_SUITE_LEAVE
 */
#define PICOTEST_SUITE_LEAVE_DEFAULT(suiteName, nb, fail)

/**
 * Define the test suite leave hook.
 *
 * Called after running all subtests.
 *
 * The default hook does nothing. Redefine this macro to use a custom hook,
 * which must follow the @ref PicoTestSuiteLeaveProc signature.
 *
 * @note Custom functions only apply to the tests defined after the macro
 * redefinition. As macros can be redefined several times, this means that
 * different functions may apply for the same source.
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_SUITE_LEAVE example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PicoTestSuiteLeaveProc
 * @see PICOTEST_SUITE_LEAVE_DEFAULT
 * @see PICOTEST_SUITE_ENTER
 */
#define PICOTEST_SUITE_LEAVE PICOTEST_SUITE_LEAVE_DEFAULT

/**
 * Function signature of test suite before subtest hooks.
 *
 * Called before running each subtest.
 *
 * @param suiteName     Test suite name.
 * @param nb            Number of subtests.
 * @param fail          Failed test suite subtests so far  (including its
 *                      subtests' subtests if any).
 * @param index         Index of subtest.
 * @param testName      Name of subtest.
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_SUITE_BEFORE_SUBTEST example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PICOTEST_SUITE_BEFORE_SUBTEST
 */
typedef void(PicoTestSuiteBeforeSubtestProc)(const char *suiteName, int nb,
                                             int fail, int index,
                                             const char *testName);

/**
 * Default test suite before subtest hook. Does nothing.
 *
 * @see PicoTestSuiteBeforeSubtestProc
 * @see PICOTEST_SUITE_BEFORE_SUBTEST
 */
#define PICOTEST_SUITE_BEFORE_SUBTEST_DEFAULT(suiteName, nb, fail, index,      \
                                              testName)

/**
 * Define the test suite before subset hook.
 *
 * The default hook does nothing. Redefine this macro to use a custom hook,
 * which must follow the @ref PicoTestSuiteBeforeSubtestProc signature.
 *
 * @note Custom functions only apply to the tests defined after the macro
 * redefinition. As macros can be redefined several times, this means that
 * different functions may apply for the same source.
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_SUITE_BEFORE_SUBTEST example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PicoTestSuiteBeforeSubtestProc
 * @see PICOTEST_SUITE_BEFORE_SUBTEST_DEFAULT
 * @see PICOTEST_SUITE_AFTER_SUBTEST
 */
#define PICOTEST_SUITE_BEFORE_SUBTEST PICOTEST_SUITE_BEFORE_SUBTEST_DEFAULT

/**
 * Function signature of test suite after subtest hooks.
 *
 * Called before running each subtest.
 *
 * @param suiteName     Test suite name.
 * @param nb            Number of subtests.
 * @param fail          Failed test suite subtests so far (including its
 *                      subtests' subtests if any).
 * @param index         Index of subtest.
 * @param testName      Name of subtest.
 * @param sfail         The subtest's failed tests (including its subtests if
 *                      any).
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_SUITE_AFTER_SUBTEST example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PICOTEST_SUITE_AFTER_SUBTEST
 */
typedef void(PicoTestSuiteAfterSubtestProc)(const char *suiteName, int nb,
                                            int fail, int index,
                                            const char *testName, int sfail);

/**
 * Default test suite after subtest hook. Does nothing.
 *
 * @see PicoTestSuiteAfterSubtestProc
 * @see PICOTEST_SUITE_AFTER_SUBTEST
 */
#define PICOTEST_SUITE_AFTER_SUBTEST_DEFAULT(suiteName, nb, fail, index,       \
                                             testName, sfail)

/**
 * Define the test suite after subset hook.
 *
 * The default hook does nothing. Redefine this macro to use a custom hook,
 * which must follow the @ref PicoTestSuiteAfterSubtestProc signature.
 *
 * @note Custom functions only apply to the tests defined after the macro
 * redefinition. As macros can be redefined several times, this means that
 * different functions may apply for the same source.
 *
 * @par Usage
 *      @snippet hooks.c    PICOTEST_SUITE_AFTER_SUBTEST example
 *
 * @par Examples
 *      @example_file{hooks.c}
 *
 * @see PicoTestSuiteAfterSubtestProc
 * @see PICOTEST_SUITE_AFTER_SUBTEST_DEFAULT
 * @see PICOTEST_SUITE_BEFORE_SUBTEST
 */
#define PICOTEST_SUITE_AFTER_SUBTEST PICOTEST_SUITE_AFTER_SUBTEST_DEFAULT

/*! \} End of Test Suite Hooks */

/*! \} End of Test Suites */

/*! \} End of Public Interface */

/*! \internal
 * \defgroup utilities Utilities
 *
 * Utility macros and building blocks.
 * \{
 */

/*! \internal
 * \name Basic Utilities
 * \{
 */

/** \internal
 * Turn argument into a C string.
 */
#define _PICOTEST_STRINGIZE(arg) #arg

/** \internal
 * Concatenate both arguments.
 */
#define _PICOTEST_CONCATENATE(arg1, arg2) _PICOTEST_CONCATENATE1(arg1, arg2)

/*! \cond IGNORE */
#define _PICOTEST_CONCATENATE1(arg1, arg2) _PICOTEST_CONCATENATE2(arg1, arg2)
#define _PICOTEST_CONCATENATE2(arg1, arg2) arg1##arg2
/*! \endcond */

/*! \} End of Basic Utilities */

/*! \internal
 * \name Variadic Macro Utilities
 *
 * Macro hackery for accessing args passed to variadic macros.
 *
 * @see
 * http://groups.google.com/group/comp.std.c/browse_thread/thread/77ee8c8f92e4a3fb/346fc464319b1ee5?pli=1
 * \{
 */

/** \internal
 * Get the number of args passed to it.
 *
 * @param ...   Arguments passed to the variadic macro.
 *
 * @warning Argument length must be between 1 and 63. Empty lists return zero
 * due to limitations of the C preprocessor.
 */
#if defined(_PICOTEST_PARENS)
#define _PICOTEST_ARGCOUNT(...)                                                \
    _PICOTEST_LASTARG _PICOTEST_PARENS(                                        \
        __VA_ARGS__, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50,   \
        49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33,    \
        32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16,    \
        15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#else
#define _PICOTEST_ARGCOUNT(...)                                                \
    _PICOTEST_LASTARG(__VA_ARGS__, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, \
                      52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39,  \
                      38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25,  \
                      24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11,  \
                      10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#endif /* defined(_PICOTEST_PARENS) */

/*! \cond IGNORE */
#define _PICOTEST_LASTARG(                                                     \
    _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16,     \
    _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, \
    _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, \
    _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, \
    _62, _63, N, ...)                                                          \
    N
/*! \endcond */

/** \internal
 * Iterate over the args passed to it.
 *
 * @param what  Function taking one argument, applied to all remaining
 *              arguments.
 * @param ...   Arguments passed to the variadic macro.
 *
 * @warning Limited to 63 arguments.
 */

#if defined(_PICOTEST_PARENS)
#define _PICOTEST_FOR_EACH(what, ...)                                          \
    _PICOTEST_CONCATENATE(_PICOTEST_FOR_EACH_,                                 \
                          _PICOTEST_ARGCOUNT(__VA_ARGS__))                     \
    _PICOTEST_PARENS(what, __VA_ARGS__)
#else
#define _PICOTEST_FOR_EACH(what, ...)                                          \
    _PICOTEST_CONCATENATE(_PICOTEST_FOR_EACH_,                                 \
                          _PICOTEST_ARGCOUNT(__VA_ARGS__))                     \
    (what, __VA_ARGS__)
#endif /* defined(_PICOTEST_PARENS) */

/*! \cond IGNORE */
#if defined(_PICOTEST_PARENS)
#define _PICOTEST_FOR_EACH_1(what, x) what(x)
#define _PICOTEST_FOR_EACH_2(what, x, ...)                                     \
    what(x) _PICOTEST_FOR_EACH_1 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_3(what, x, ...)                                     \
    what(x) _PICOTEST_FOR_EACH_2 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_4(what, x, ...)                                     \
    what(x) _PICOTEST_FOR_EACH_3 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_5(what, x, ...)                                     \
    what(x) _PICOTEST_FOR_EACH_4 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_6(what, x, ...)                                     \
    what(x) _PICOTEST_FOR_EACH_5 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_7(what, x, ...)                                     \
    what(x) _PICOTEST_FOR_EACH_6 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_8(what, x, ...)                                     \
    what(x) _PICOTEST_FOR_EACH_7 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_9(what, x, ...)                                     \
    what(x) _PICOTEST_FOR_EACH_8 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_10(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_9 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_11(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_10 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_12(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_11 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_13(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_12 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_14(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_13 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_15(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_14 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_16(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_15 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_17(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_16 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_18(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_17 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_19(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_18 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_20(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_19 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_21(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_20 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_22(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_21 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_23(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_22 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_24(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_23 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_25(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_24 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_26(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_25 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_27(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_26 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_28(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_27 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_29(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_28 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_30(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_29 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_31(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_30 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_32(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_31 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_33(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_32 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_34(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_33 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_35(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_34 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_36(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_35 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_37(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_36 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_38(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_37 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_39(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_38 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_40(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_39 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_41(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_40 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_42(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_41 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_43(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_42 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_44(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_43 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_45(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_44 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_46(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_45 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_47(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_46 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_48(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_47 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_49(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_48 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_50(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_49 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_51(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_50 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_52(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_51 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_53(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_52 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_54(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_53 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_55(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_54 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_56(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_55 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_57(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_56 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_58(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_57 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_59(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_58 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_60(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_59 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_61(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_60 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_62(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_61 _PICOTEST_PARENS(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_63(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_62 _PICOTEST_PARENS(what, __VA_ARGS__)
#else
#define _PICOTEST_FOR_EACH_1(what, x) what(x)
#define _PICOTEST_FOR_EACH_2(what, x, ...)                                     \
    what(x) _PICOTEST_FOR_EACH_1(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_3(what, x, ...)                                     \
    what(x) _PICOTEST_FOR_EACH_2(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_4(what, x, ...)                                     \
    what(x) _PICOTEST_FOR_EACH_3(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_5(what, x, ...)                                     \
    what(x) _PICOTEST_FOR_EACH_4(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_6(what, x, ...)                                     \
    what(x) _PICOTEST_FOR_EACH_5(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_7(what, x, ...)                                     \
    what(x) _PICOTEST_FOR_EACH_6(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_8(what, x, ...)                                     \
    what(x) _PICOTEST_FOR_EACH_7(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_9(what, x, ...)                                     \
    what(x) _PICOTEST_FOR_EACH_8(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_10(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_9(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_11(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_10(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_12(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_11(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_13(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_12(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_14(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_13(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_15(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_14(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_16(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_15(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_17(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_16(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_18(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_17(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_19(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_18(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_20(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_19(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_21(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_20(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_22(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_21(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_23(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_22(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_24(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_23(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_25(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_24(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_26(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_25(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_27(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_26(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_28(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_27(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_29(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_28(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_30(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_29(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_31(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_30(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_32(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_31(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_33(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_32(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_34(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_33(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_35(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_34(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_36(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_35(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_37(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_36(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_38(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_37(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_39(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_38(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_40(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_39(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_41(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_40(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_42(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_41(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_43(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_42(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_44(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_43(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_45(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_44(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_46(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_45(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_47(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_46(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_48(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_47(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_49(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_48(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_50(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_49(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_51(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_50(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_52(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_51(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_53(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_52(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_54(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_53(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_55(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_54(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_56(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_55(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_57(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_56(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_58(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_57(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_59(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_58(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_60(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_59(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_61(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_60(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_62(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_61(what, __VA_ARGS__)
#define _PICOTEST_FOR_EACH_63(what, x, ...)                                    \
    what(x) _PICOTEST_FOR_EACH_62(what, __VA_ARGS__)
#endif /* defined(_PICOTEST_PARENS) */
/*! \endcond */

/*! \} End of Variadic Macro Utilities */

/*! \} End of Utilities */

#endif /* _PICOTEST */
