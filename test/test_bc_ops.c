#include "buffer_chain.h"
#include "testing.h"


TEST_SUITE("bufferchain operations")

	buffer_chain compare_chain1 = {"test", 5, 0, NULL, STATIC};
	buffer_chain compare_chain2_link2 = {"st", 2, 0, NULL, STATIC};
	buffer_chain compare_chain2 = {"te", 2, 0, &compare_chain2_link2, STATIC};
	buffer_chain compare_chain3 = {"test\x00weee", 9, 0, NULL, STATIC};
	buffer_chain compare_chain4 = {"TEST", 5, 0, NULL, STATIC};
	buffer_chain compare_offset = {"xxxtest", 7, 3, NULL, STATIC};
	buffer_chain search_chain1 = {"abcdefgh\n", 9, 0, NULL, STATIC};
	buffer_chain search_chain2_link2 = {"ghijk\n", 6, 0, NULL, STATIC};
	buffer_chain search_chain2 = {"xxxabcdef", 9, 3, &search_chain2_link2, STATIC};
	buffer_chain search_chain3_link3 = {"ghijkl\n", 7, 0, NULL, STATIC};
	buffer_chain search_chain3_link2 = {"f", 1, 0, &search_chain3_link3, STATIC};
	buffer_chain search_chain3 = {"xxxabcde", 8, 3, &search_chain3_link2, STATIC};

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

	TEST_CASE("bc_memchr checks")

		buffer_chain *match, *match2;
		match = bc_memchr(&compare_chain1, 't');
		assert_ptr_equality("first byte matches buffer", match->buffer,
		                    compare_chain1.buffer);
		assert_size_equality("no offset for first byte match", match->offset,
		                     (size_t)0);
		free(match);

		match = bc_memchr(&compare_chain1, 'x');
		assert_ptr_equality("null pointer returned for char not found",
		                    match, NULL);

		match = bc_memchr(&compare_chain2, 's');
		assert_ptr_equality("second chain matched", match->buffer,
		                    compare_chain2_link2.buffer);
		assert_size_equality("offset in new chain starts at 0", match->offset,
		                     (size_t)0);

		match2 = bc_memchr(match, 't');
		assert_ptr_equality("can match from other matches", match2->buffer,
		                    compare_chain2_link2.buffer);
		assert_size_equality("offset for this match is 1", match2->offset,
		                     (size_t)1);
		free(match);
		free(match2);

	TEST_CASE_END

	TEST_CASE("bc_getdelim checks")

		size_t len;
		char *getstring;
		getstring = bc_getdelim(&search_chain1, 'x', &len);
		assert_ptr_equality("Null returned for delim not found", getstring,
		                    NULL);
		assert_size_equality("len 0 for match not found", len, (size_t)0);

		getstring = bc_getdelim(&search_chain1, '\n', &len);
		assert_int_equality("chain1 matches",
		                    strncmp("abcdefgh\n", getstring, 9), 0);
		assert_size_equality("length for first match is 9",
		                     len, (size_t)9);
		free(getstring);

		getstring = bc_getdelim(&search_chain1, 'd', &len);
		assert_int_equality("chain1 matches mid delim",
		                    strncmp("abcd", getstring, 4), 0);
		assert_size_equality("length for this match is 4",
		                     len, (size_t)4);
		free(getstring);

		getstring = bc_getdelim(&search_chain2, '\n', &len);
		assert_int_equality("chain2 full match matches",
		                    strncmp("abcdefghijk\n", getstring, 12), 0);
		assert_size_equality("length for second match is 12", len, (size_t)12);
		free(getstring);

		getstring = bc_getdelim(&search_chain2, 'f', &len);
		assert_int_equality("chain2 matches mid delim",
		                    strcmp("abcdef", getstring), 0);
		assert_size_equality("length for mid chain2 match is 8",
		                     len, (size_t)6);
		free(getstring);

	TEST_CASE_END

	TEST_CASE("bc_memstr checks")

		buffer_chain *mem_match;
		mem_match = bc_memstr(&search_chain3, "test");
		assert_ptr_equality("null returned for no match", mem_match, NULL);

		mem_match = bc_memstr(&search_chain3, "e");
		assert_ptr_equality("single char match in first buffer",
		                    mem_match->buffer, search_chain3.buffer);
		assert_size_equality("offset for this match is 7", mem_match->offset,
		                     (size_t)7);
		free(mem_match);

		mem_match = bc_memstr(&search_chain3, "ab");
		assert_ptr_equality("multi char match in first buffer",
		                    mem_match->buffer, search_chain3.buffer);
		assert_size_equality("offset for this match is 3", mem_match->offset,
		                     (size_t)3);
		free(mem_match);

		mem_match = bc_memstr(&search_chain3, "def");
		assert_ptr_equality("multi char match in across 2 links",
		                    mem_match->buffer, search_chain3.buffer);
		assert_size_equality("offset for this match is 6", mem_match->offset,
		                     (size_t)6);
		free(mem_match);

		mem_match = bc_memstr(&search_chain3, "defg");
		assert_ptr_equality("multi char match in across 3 links",
		                    mem_match->buffer, search_chain3.buffer);
		assert_size_equality("offset for this match is 6", mem_match->offset,
		                     (size_t)6);
		free(mem_match);

		mem_match = bc_memstr(&search_chain3, "fg");
		assert_ptr_equality("multi char match in across 3 links",
		                    mem_match->buffer, search_chain3_link2.buffer);
		assert_size_equality("offset for this match is 0", mem_match->offset,
		                     (size_t)0);
		free(mem_match);

	TEST_CASE_END

TEST_SUITE_END
