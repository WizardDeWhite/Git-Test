#include <stdio.h>
#include "types.h"
#include "bitmap.h"
#include "bitops.h"
#include "test_common.h"
#include "__ffs.h"
#include "__fls.h"

#define TESTS_BITS 35
DECLARE_BITMAP(bitmaps, TESTS_BITS);

void bitmap_zero_checks()
{
	int i;

	prefix_reset();
	prefix_push("bitmap_zero");
	test_print("Running %s tests...\n", "bitmap_zero");

	PREFIX_PUSH();

	bitmap_zero(bitmaps, TESTS_BITS);

	for(i = 0; i < TESTS_BITS; i++)
		ASSERT_FALSE(test_bit(i, bitmaps));

	test_pass_pop();

	prefix_pop();
}

void bitmap_fill_checks()
{
	int i;

	prefix_reset();
	prefix_push("bitmap_fill");
	test_print("Running %s tests...\n", "bitmap_fill");

	PREFIX_PUSH();

	bitmap_fill(bitmaps, TESTS_BITS);

	for(i = 0; i < TESTS_BITS; i++)
		ASSERT_TRUE(test_bit(i, bitmaps));

	test_pass_pop();

	prefix_pop();
}

void bitmap_clear_one_check()
{
	int i;

	PREFIX_PUSH();

	bitmap_fill(bitmaps, TESTS_BITS);

	for(i = 0; i < TESTS_BITS; i++) {
		ASSERT_TRUE(test_bit(i, bitmaps));
		bitmap_clear(bitmaps, i, 1);
		ASSERT_FALSE(test_bit(i, bitmaps));
	}

	test_pass_pop();

}

void bitmap_clear_two_check()
{
	int i;

	PREFIX_PUSH();

	bitmap_fill(bitmaps, TESTS_BITS);

	for(i = 0; i < TESTS_BITS - 1; i +=2 ) {
		ASSERT_TRUE(test_bit(i, bitmaps));
		ASSERT_TRUE(test_bit(i + 1, bitmaps));
		bitmap_clear(bitmaps, i, 2);
		ASSERT_FALSE(test_bit(i, bitmaps));
		ASSERT_FALSE(test_bit(i + 1, bitmaps));
	}

	test_pass_pop();
}

void bitmap_first_last_word_mask_check()
{
	int i;

	prefix_reset();
	prefix_push("bitmap_fist_last_word");
	test_print("Running %s tests...\n", "bitmap_fist_last_word");

	PREFIX_PUSH();

	for (i = 0; i < BITS_PER_LONG * 2; i++) {
		/* Special case for 0, which both mask are (~0UL) */
		if ((i % BITS_PER_LONG) == 0) {
			ASSERT_EQ((~0UL), BITMAP_FIRST_WORD_MASK(i));
			ASSERT_EQ((~0UL), BITMAP_LAST_WORD_MASK(i));
		} else {
			/* Otherwise, FIRST and LAST word mask are MECE */
			ASSERT_EQ((~0UL),
				BITMAP_FIRST_WORD_MASK(i) | BITMAP_LAST_WORD_MASK(i));
			ASSERT_EQ(0,
				BITMAP_FIRST_WORD_MASK(i) & BITMAP_LAST_WORD_MASK(i));
		}
	}

	test_pass_pop();

	prefix_pop();
}

void bitmap_clear_checks()
{
	prefix_reset();
	prefix_push("bitmap_clear");
	test_print("Running %s tests...\n", "bitmap_clear");

	bitmap_clear_one_check();
	bitmap_clear_two_check();

	prefix_pop();
}

/* calculate the maximum number of continuous 1 */
int maxstr1(unsigned x)
{
	int k;

	for (k = 0; x != 0; k++)
		x = x & 2*x;
	return k;
}

void maxstr1_check()
{
	prefix_reset();
	prefix_push("maxstr1");
	test_print("Running %s tests...\n", "maxstr1");

	ASSERT_EQ(8, maxstr1(0xff0000ff));
	ASSERT_EQ(8, maxstr1(0xff1000ff));
	ASSERT_EQ(9, maxstr1(0xff8000ff));
	ASSERT_EQ(9, maxstr1(0xff1001ff));

	test_pass_pop();

	prefix_pop();
}

void bitmap_ffs_check()
{
	PREFIX_PUSH();

	ASSERT_EQ(0, __ffs(1));
	ASSERT_EQ(7, __ffs(0x80));
	ASSERT_EQ(31, __ffs(0x80000000));
	ASSERT_EQ(32, __ffs(0x100000000));
	ASSERT_EQ(0, __ffs(0x8000000000000001));
	ASSERT_EQ(63, __ffs(0x8000000000000000));

	test_pass_pop();
}

void bitmap_fls_check()
{
	PREFIX_PUSH();

	ASSERT_EQ(1, __fls(1));
	ASSERT_EQ(32, __fls(0x80000000));
	ASSERT_EQ(32, __fls(0x80000001));
	ASSERT_EQ(64, __fls(0x8000000100000001));

	test_pass_pop();
}

void bitmap_ffs_checks()
{
	prefix_reset();
	prefix_push("ffs");
	test_print("Running %s tests...\n", "ffs");

	bitmap_ffs_check();
	bitmap_fls_check();

	prefix_pop();
}

/* return the value of the lowest set bit
 *
 * For example, 0x3204 lowest set bit is 0x4
 */
int lowest_bit_value(int x)
{
	/* could be 1<<(ffs(x)-1) too*/
	return x & ~(x-1);
}

void lowest_bit_value_check()
{

	PREFIX_PUSH();

	ASSERT_EQ(0x4, lowest_bit_value(0x3204));
	ASSERT_EQ(0x1, lowest_bit_value(0x3205));

	test_pass_pop();
}

/* could be optimized? */
int highest_bit_value(int x)
{
	return 1 << (__fls(x) - 1);
}

void highest_bit_value_check()
{

	PREFIX_PUSH();

	ASSERT_EQ(0x2000, highest_bit_value(0x3001));
	ASSERT_EQ(0x2000, highest_bit_value(0x2001));
	ASSERT_EQ(0x8000, highest_bit_value(0x8001));
	ASSERT_EQ(0x8000, highest_bit_value(0xF001));

	test_pass_pop();
}

void bitmap_lowest_highest_bit_value_check()
{
	prefix_reset();
	prefix_push("bitmap_lowest_highest_bit_value_check");
	test_print("Running %s tests...\n", "bitmap_lowest_highest_bit_value_check");

	lowest_bit_value_check();
	highest_bit_value_check();

	prefix_pop();
}

int main(int argc, char *argv[])
{
	parse_args(argc, argv);

	bitmap_zero_checks();
	bitmap_fill_checks();
	bitmap_clear_checks();
	bitmap_first_last_word_mask_check();
	maxstr1_check();
	bitmap_ffs_checks();
	bitmap_lowest_highest_bit_value_check();
	return 0;
}
