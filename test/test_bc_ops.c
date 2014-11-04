#include "buffer_chain.h"
#include "testing.h"


TEST_SUITE("bufferchain operations")

	buffer_chain compare_chain1 = {"test", 5, 0, NULL, STATIC};
	buffer_chain compare_chain2_link2 = {"st", 2, 0, NULL, STATIC};
	buffer_chain compare_chain2 = {"te", 2, 0, &compare_chain2_link2, STATIC};
	buffer_chain compare_chain3 = {"test\x00weee", 9, 0, NULL, STATIC};
	buffer_chain compare_chain4 = {"TEST", 5, 0, NULL, STATIC};
	buffer_chain compare_offset = {"xxxtest", 7, 3, NULL, STATIC};

	TEST_CASE("bc_strncmp checks")

		// Basic usage test cases
		assert_int_equality("matched compare returns 0",
		                    bc_strncmp("test", &compare_chain1, 4), 0);
		assert_int_equality("lower value returns -1",
		                    bc_strncmp("tesa", &compare_chain1, 4), -1);
		assert_int_equality("greater value returns 1",
		                    bc_strncmp("tesx", &compare_chain1, 4), 1);
		assert_int_equality("can compare null values as well",
		                    bc_strncmp("test\x00weee", &compare_chain3, 9), 0);

		// Test compare string longer than buffer chain
		assert_int_equality("pattern longer than buffer returns 1",
		                    bc_strncmp("testandthenmoretest", &compare_chain1,
		                               19),
		                    1);

		// Test cases that follow the buffer chain
		assert_int_equality("chained, match returns 0",
		                    bc_strncmp("test", &compare_chain2, 4), 0);
		assert_int_equality("chained, lower value returns -1",
		                    bc_strncmp("tesa", &compare_chain2, 4), -1);
		assert_int_equality("chained, greater value returns 1",
		                    bc_strncmp("tesx", &compare_chain2, 4), 1);

		// Test cases with offset
		assert_int_equality("offset, match returns 0",
		                    bc_strncmp("test", &compare_offset, 4), 0);
		assert_int_equality("offset, lower value returns -1",
		                    bc_strncmp("tesa", &compare_offset, 4), -1);
		assert_int_equality("offset, greater value returns 1",
		                    bc_strncmp("tesx", &compare_offset, 4), 1);

		// Test case sensitivity
		assert_int_equality("capital letters have lower value",
		                    bc_strncmp("tesX", &compare_chain1, 4), -1);

	TEST_CASE_END

	TEST_CASE("bc_strncasecmp checks")

		// Basic test cases
		assert_int_equality("exact match returns 0",
		                    bc_strncasecmp("test", &compare_chain1, 4), 0);
		assert_int_equality("case insensitive, lower value returns -1",
		                    bc_strncasecmp("tesa", &compare_chain4, 4), -1);
		assert_int_equality("case insensitive, greater value returns 1",
		                    bc_strncasecmp("TESX", &compare_chain1, 4), 1);

		// Chained test cases
		assert_int_equality("chained, match returns 0",
		                    bc_strncasecmp("TEST", &compare_chain2, 4), 0);
		assert_int_equality("chained, lower value returns -1",
		                    bc_strncasecmp("TESA", &compare_chain2, 4), -1);
		assert_int_equality("chained, greater value returns 1",
		                    bc_strncasecmp("TESX", &compare_chain2, 4), 1);

		// Test cases with offset
		assert_int_equality("offset, match returns 0",
		                    bc_strncasecmp("TEST", &compare_offset, 4), 0);
		assert_int_equality("offset, lower value returns -1",
		                    bc_strncasecmp("TESA", &compare_offset, 4), -1);
		assert_int_equality("offset, greater value returns 1",
		                    bc_strncasecmp("TESX", &compare_offset, 4), 1);

	TEST_CASE_END

TEST_SUITE_END
