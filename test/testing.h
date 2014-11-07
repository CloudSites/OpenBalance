#ifndef __TESTING_H__
#define __TESTING_H__

#include <stdio.h>

#define TEST_SUITE(name) int main(int argc, char *argv[]) \
{ \
	int testsuite_status = 0; \
	int testcase_count = 0; \
	int assertion_count = 0; \
	int format_offset = 0;\
	printf(" /============================================================================\\\n"); \
	printf(" | Starting test suite: %-53.53s |\n", name); \
	printf(" +============================================================================+\n\n");

#define TEST_SUITE_END \
	printf(" +============================================================================+\n"); \
	if(!assertion_count) \
	{ \
		printf(" | No assertions made in tests ran                                            |\n");  \
	} \
	else if(testsuite_status) \
	{ \
		printf(" | Failures present in test suite                                             |\n"); \
	} \
	else \
	{ \
		printf(" | All tests passed                                                           |\n"); \
	} \
	printf(" | %d %s ran from %d %s%n", assertion_count, (assertion_count == 1) ? "assertion" : "assertions", testcase_count, (testcase_count == 1) ? "case" : "cases", &format_offset); \
	while(format_offset++ < 78) \
	{ \
		printf(" ");\
	} \
	printf("|\n \\============================================================================/\n"); \
	return testsuite_status; \
}

#define TEST_CASE(name) testcase_count++; \
	printf("  /--------------------------------------------------------------------------\\\n"); \
	printf("  | Starting test case: %-52.52s |\n", name); \
	printf("  +--------------------------------------------------------------------------+\n");

#define TEST_CASE_END \
	printf("  +--------------------------------------------------------------------------+\n"); \
	printf("  | done.                                                                    |\n"); \
	printf("  \\--------------------------------------------------------------------------/\n\n");

#define assert_true(assert_comment, something) assertion_count++; \
if(!something) \
{ \
	printf("Assertion failed: %s\n'" #something "' is not true.\n", assert_comment); \
	testsuite_status = 1; \
}

#define assert_false(assert_comment, something) assertion_count++; \
if(something) \
{ \
	printf("Assertion failed: %s\n'" #something "' is not false.\n", assert_comment); \
	testsuite_status = 1; \
}


#define assert_int_equality(assert_comment, something, asserted_value) assertion_count++; \
if(something != asserted_value) \
{ \
	printf("Assertion failed: %s\n'" #something "' is not %d. Is %d\n", \
	       assert_comment, asserted_value, something); \
	testsuite_status = 1; \
}

#define assert_size_equality(assert_comment, something, asserted_value) assertion_count++; \
if(something != asserted_value) \
{ \
	printf("Assertion failed: %s\n'" #something "' is not %zd. Is %zd\n", \
	       assert_comment, asserted_value, something); \
	testsuite_status = 1; \
}

#define assert_ptr_equality(assert_comment, something, asserted_value) assertion_count++; \
if(something != asserted_value) \
{ \
	printf("Assertion failed: %s\n'" #something "' is not %p. Is %p\n", \
	       assert_comment, asserted_value, something); \
	testsuite_status = 1; \
}

#endif
