#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include "utf8decoder.h"

/* Unicode character validity tests */
#define is_in_range(c, lo, hi) ((c >= lo) && (c <= hi))
#define is_surrogate(c) is_in_range(c, 0xd800, 0xdfff)
#define is_noncharacter(c) is_in_range(c, 0xfdd0, 0xfdef)
#define is_reserved(c) ((c & 0xfffe) == 0xfffe)
#define is_outofrange(c) (c > 0x10ffff)
#define is_invalid(c) (is_reserved(c) || is_outofrange(c) || is_noncharacter(c) || is_surrogate(c))

/* encode a UTF-32 character to UTF-8 */
static inline size_t
encode_utf8(uint32_t c, uint8_t *out) {
	static const uint8_t mask[] = {0xc0, 0xe0, 0xf0, 0xf8, 0xfc};
	size_t len;
	uint32_t t;

	if (c < 0x80) {
		out[0] = c;
		return 1;
	} else {
		t = c;
		len = 0;
		if (t >= 0x10000) { out[3] = (c | 0x80) & 0xbf; c >>= 6; len += 1; }
		if (t >= 0x800)   { out[2] = (c | 0x80) & 0xbf; c >>= 6; len += 1; }
						  { out[1] = (c | 0x80) & 0xbf; c >>= 6; }
						  { out[0] = (c | mask[len]); }
		return len + 2;
	}
}

/* decode a UTF-16 stream to UTF-32, no error checking */
static inline size_t
decode_utf16(uint16_t *c, uint32_t *out) {
	if (is_in_range(c[0], 0xd800, 0xdbff)) {
		out[0] = (((uint32_t)(c[0] & 0x3ff) << 10) | (c[1] & 0x3ff)) + 0x10000;
		return 2;
	} else {
		out[0] = c[0];
		return 1;
	}
}

/* test encoding and decoding of every value from 0x0 to 0x110000 (1 above maximum) */
static void
test_full_range() {
	uint32_t i, utf32, converted[4];
	uint16_t utf16[4];
	uint8_t utf8[4];
	size_t len8, len16, len32;

	for (i = 0; i <= 0x110000; i++) {
		utf32 = (is_invalid(i)) ? utf_replacement : i; /* the expected character */
		len8 = encode_utf8(i, utf8); /* encode the (possibly invalid) character */

		/* utf16 */
		len16 = utf8_to_utf16(utf8, len8, utf16); /* decode the (possibly invalid) chracter */
		decode_utf16(utf16, converted);
		if (utf32 != converted[0])
			printf("UTF32->UTF8->UTF16: Mismatch at %x, WANT: %x, GOT %x\n", i, utf32, converted[0]);

		/* utf32 */
		len32 = utf8_to_utf32(utf8, len8, converted); /* decode the (possibly invalid) chracter */
		if (utf32 != converted[0])
			printf("UTF32->UTF8->UTF32: Mismatch at %x, WANT: %x, GOT %x\n", i, utf32, converted[0]);
	}
}

/* test encoding and decoding of every value from 0x0 to 0x110000 (1 above maximum), streamed */
static void
test_full_range_onepass() {
	uint32_t *utf32, *utf32_wanted, *utf32_pos;
	uint16_t *utf16, *utf16_pos;
	uint8_t *utf8, *utf8_pos;
	uint32_t i;
	size_t len8, len16, len32, pos, random_len;
	size_t utf8_left, read, written;
	utf8_decode_state incremental_state;

	/* generate utf8 buffer and correct utf32 buffer */
	utf32_wanted = (uint32_t *)malloc(0x110001 * sizeof(uint32_t));
	utf8 = (uint8_t *)malloc(4 * 0x110001); /* maximum spaced needed */

	for (i = 0, pos = 0; i < 0x110001; i++) {
		utf32_wanted[i] = (is_invalid(i)) ? utf_replacement : i; /* the expected character */
		len8 = encode_utf8(i, utf8 + pos); /* encode the (possibly invalid) character */
		pos += len8;
	}

	utf32 = (uint32_t *)malloc(max_output_utf8_to_utf32_bytes(pos)); /* maximum spaced needed */


	/* utf16 */
	utf16 = (uint16_t *)malloc(max_output_utf8_to_utf16_bytes(pos)); /* maximum spaced needed */
	memset(utf16, 0, max_output_utf8_to_utf16_bytes(pos));

	/* convert utf8 to utf16, then utf16 to utf32 */
	len16 = utf8_to_utf16(utf8, pos, utf16);
	for (read = 0, written = 0; read < len16;) {
		read += decode_utf16(utf16 + read, utf32 + written);
		written += 1;
	}
	if (written != 0x110001)
		printf("UTF32->UTF8->UTF16: One pass conversion resulted in %x characters, wanted %x\n", (uint32_t)written, i);
	else if (memcmp(utf32_wanted, utf32, written * sizeof(uint32_t)) != 0)
		printf("UTF32->UTF8->UTF16: One pass conversion didn't match expected values\n");

	/* incremental utf8->utf16 */
	utf8_pos = utf8;
	utf8_left = pos;
	utf16_pos = utf16;
	memset(utf16, 0, max_output_utf8_to_utf16_bytes(pos));
	utf8_to_utf16_init(&incremental_state);
	while (utf8_left) {
		random_len = (((((utf8_left * 0xcafefade) >> 7) * 0xbeeffeed) >> 24) & 31) + 1;
		utf8_to_utf16_continue(&incremental_state, utf8_pos, (random_len <= utf8_left) ? random_len : utf8_left, &read, utf16_pos, &written);
		utf8_left -= read;
		utf8_pos += read;
		utf16_pos += written;
	}
	utf8_to_utf16_finish(&incremental_state, utf16_pos, &written);
	utf16_pos += written;
	len16 = utf16_pos - utf16;

	for (read = 0, written = 0; read < len16;) {
		read += decode_utf16(utf16 + read, utf32 + written);
		written += 1;
	}
	len32 = written;
	if (len32 != 0x110001)
		printf("UTF32->UTF8->UTF16: Incremental conversion resulted in %x characters, wanted %x\n", (uint32_t)len32, i);
	else if (memcmp(utf32_wanted, utf32, len32 * sizeof(uint32_t)) != 0)
		printf("UTF32->UTF8->UTF16: Incremental conversion didn't match expected values\n");

	free(utf16);




	/* utf32 */
	memset(utf32, 0, max_output_utf8_to_utf32_bytes(pos));

	/* convert utf8 to utf32 */
	len32 = utf8_to_utf32(utf8, pos, utf32);
	if (len32 != 0x110001)
		printf("UTF32->UTF8->UTF32: One pass conversion resulted in %x characters, wanted %x\n", (uint32_t)len32, i);
	else if (memcmp(utf32_wanted, utf32, len32 * sizeof(uint32_t)) != 0)
		printf("UTF32->UTF8->UTF32: One pass conversion didn't match expected values\n");

	/* incremental utf8->utf32 */
	utf8_pos = utf8;
	utf8_left = pos;
	utf32_pos = utf32;
	memset(utf32, 0, max_output_utf8_to_utf32_bytes(pos));
	utf8_to_utf32_init(&incremental_state);
	while (utf8_left) {
		random_len = (((((utf8_left * 0xdeadbeef) >> 8) * 0xcafebabe) >> 24) & 31) + 1;
		utf8_to_utf32_continue(&incremental_state, utf8_pos, (random_len <= utf8_left) ? random_len : utf8_left, &read, utf32_pos, &written);
		utf8_left -= read;
		utf8_pos += read;
		utf32_pos += written;
	}
	utf8_to_utf32_finish(&incremental_state, utf32_pos, &written);
	utf32_pos += written;

	len32 = utf32_pos - utf32;
	if (len32 != 0x110001)
		printf("UTF32->UTF8->UTF32: Incremental conversion resulted in %x characters, wanted %x\n", (uint32_t)len32, i);
	else if (memcmp(utf32_wanted, utf32, len32 * sizeof(uint32_t)) != 0)
		printf("UTF32->UTF8->UTF32: Incremental conversion didn't match expected values\n");

	free(utf32_wanted);
	free(utf32);
	free(utf8);
}


/* test decoding of all overlong sequences (including 5 and 6 byte sequences) */
static void
test_overlong() {
	static const uint8_t masks[] = {0xc0, 0xe0, 0xf0, 0xf8, 0xfc};
	static const uint32_t highest_overlong[] = {0x7f, 0x7ff, 0xffff, 0x1fffff, 0x3ffffff};

	uint32_t i, j, len, val;
	uint32_t c, converted[6];
	uint8_t utf8[6];

	for (i = 0; i < 5; i++) {
		len = i + 2;
		for (val = 0; val < highest_overlong[i]; val++) {
			if (val == utf_replacement)
				continue;

			c = val;
			for (j = len; j != 0; j--) {
				utf8[j - 1] = (c | 0x80) & 0xbf;
				c >>= 6;
			}
			utf8[0] = (c | masks[i]);

			utf8_to_utf32(utf8, len, converted);
			if (converted[0] == val)
				printf("UTF8 %u bytes: Overlong encoded value %x successfully decoded!\n", len, val);
			else if (converted[0] != utf_replacement)
				printf("UTF8 %u bytes: Overlong encoded value %x decoded incorrectly!\n", len, val);
		}
	}
}

/* invalid single bytes */
static void
test_invalid_single_bytes() {
	uint32_t i, converted;
	uint8_t utf8[1];

	for (i = 0x80; i <= 0xff; i++) {
		utf8[0] = i;
		utf8_to_utf32(utf8, 1, &converted);
		if (converted != utf_replacement)
			printf("UTF8: Invalid byte value %x decoded improperly to %x!\n", i, converted);
	}
}


int main() {
	utf8_unpack_tables();

	test_full_range();
	test_full_range_onepass();
	test_overlong();
	test_invalid_single_bytes();

	return 0;
}