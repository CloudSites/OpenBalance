#ifndef __TESTING_H__
#define __TESTING_H__

#include <stdio.h>

#define TEST_SUITE(name) int main(int argc, char *argv[]) \
{ \
	int testsuite_status = 0; \
	int testcase_count = 0; \
	int assertion_count = 0; \
	printf("/=========================================================\\\n"); \
	printf("|  Starting test suite %s\n", name); \
	printf("+=========================================================+\n");

#define TEST_SUITE_END \
	if(!assertion_count) \
	{ \
		printf("| No assertions made in tests ran ");  \
	} \
	else if(testsuite_status) \
	{ \
		printf("| Failures present in test suite "); \
	} \
	else \
	{ \
		printf("| All tests passed "); \
	} \
	if(assertion_count == 1) \
	{ \
		printf("(1 assertion ran from "); \
	} \
	else \
	{ \
		printf("(%d assertions ran from ", assertion_count);\
	} \
	if(testcase_count == 1) \
	{ \
		printf("1 case)\n"); \
	} \
	else \
	{ \
		printf("%d cases)\n", testcase_count);\
	} \
	printf("\\=========================================================/\n"); \
	return testsuite_status; \
}

#define TEST_CASE(name) testcase_count++; \
	printf(" /-------------------------------------------------------\\\n"); \
	printf(" | Starting test case %s\n", name); \
	printf(" +-------------------------------------------------------+\n");

#define TEST_CASE_END \
	printf(" \\-------------------------------------------------------/\n");

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
