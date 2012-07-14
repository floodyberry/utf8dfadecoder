#include "utf8decoder.h"

/*
	Starting from:

	LeadingByteValue[256] = {
		Standard UTF-8 leading byte value lookup...
	}

	with 25 character types, Type = CharacterTypes[c]:

	CharacterTypes[256] = {
		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
		 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4,
		 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6,
		 7, 7, 7, 7, 7, 7, 7, 8, 7, 7, 7, 7, 7, 7, 9,10,
		11,11,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		13,14,14,14,14,14,14,14,14,14,14,14,14,15,16,17,
		18,19,19,19,20,21,21,21,22,22,22,22,23,23,24,24,
	}

	and 21 states, with the transition from each state to the next state indexed by character type:

	StateTransitions[21 * 25] = {
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 3,10,11,12,13,14,19,18,20, 7, 8, 9, 1,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 5, 5, 5, 5, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 3, 3, 3, 3, 5, 5, 5, 5, 5, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 3, 3, 3, 3, 3, 3, 3,15, 3,16, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 3, 3, 3, 3, 3, 3, 3, 3, 3,16, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 4,17, 4,17, 4,17, 4, 4, 4,17, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 6, 6, 4,17, 4,17, 4, 4, 4,17, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 4,17, 6, 6, 6, 6, 6, 6, 6, 6, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
	}

	where:

	State 0 = Accept
	State 1 = Reject (properly encoded UTF-8 but invalid value)
	State 2 = Fail (unexpected byte in UTF-8 stream)

	and to start:

	Type = CharacterType[c]
	Codepoint = LeadingByteValue[c]
	State = StateTransitions[(0 * 25) + Type]

	and while (State > Fail):

	Type = CharacterType[c]
	Codepoint = (Codepoint << 6) + (c & 0x3f)
	State = StateTransitions[(State * 25) + Type]

	States can be pre-multiplied by 25, but that will require 16 bits
	versus 8

	Instead of separate type & transition tables, they can be combined
	so a single lookup is used. Store the table as 256 * 21, and have each
	256 byte array per state be the mapping of the character type to
	the next state.
*/


/* pre-multiply the table by 256? avoids a mult by 256 in the lookup, but causes
   the table to double in size */

/* #define UTF8_PREMULTIPLIED_TABLE */

#if defined(UTF8_PREMULTIPLIED_TABLE)
#define UTF8_TABLE_TYPE uint16_t
#define UTF8_TABLE_PRECALC_MULTIPLIER 256
#define UTF8_TABLE_MULTIPLIER 1
#else
#define UTF8_TABLE_TYPE uint8_t
#define UTF8_TABLE_PRECALC_MULTIPLIER 1
#define UTF8_TABLE_MULTIPLIER 256
#endif

static uint8_t utf8_leading_byte_value[256];
static UTF8_TABLE_TYPE utf8_state_table[256 * 21];

/*
	This is the 256 * 21 table, run length compressed,
	packed in to a bitstream as a 5 bit value and 5 bit run
	length. Run lengths over 20 look up their length in
	utf8_state_table_lengths, and a value of 30 indicates
	a run length of 640

	107 bytes of lookup tables + unpacked code vs storing
	the 5376 byte table.
*/

static const uint16_t utf8_state_table_packed[49] = {
	0x8780,0xc45d,0x2aa8,0xc62c,0x0b42,0xcc2e,0x0720,0x670d,
	0x9220,0x1044,0x83c2,0xfa2d,0xa2d8,0x2d87,0xd97a,0x9ba2,
	0xfa2d,0xa2d9,0x2da3,0xb17a,0x8ac3,0xb63e,0xc3e8,0x2b16,
	0xd8fa,0x0fa2,0xc2fc,0x3038,0x0e88,0xb060,0x8a00,0x720e,
	0xa210,0x0d0f,0xe883,0xc5e4,0x5e40,0xe40c,0x40c5,0x0c5e,
	0x1ba2,0x5e48,0xe40c,0x40c5,0x0c5e,0x93a2,0x8317,0x62b9,
	0x0003
};

static const uint8_t utf8_state_table_lengths[9] = {
	0x1e,0x20,0x30,0x37,0x3e,0x3f,0x40,0x80,0xc0
};

#define utf8_accept (0 * UTF8_TABLE_PRECALC_MULTIPLIER)
#define utf8_reject (1 * UTF8_TABLE_PRECALC_MULTIPLIER)
#define utf8_fail   (2 * UTF8_TABLE_PRECALC_MULTIPLIER)

/* generate the tables used by the decoder */
void
utf8_unpack_tables(void) {
	uint32_t i, j, c, count, val, p;
	size_t bitsleft;

	/* generate the leading byte values */
	for (c = 0, i = 128; i != 0; i >>= 1)
		for (j = 0; j < i; j++)
			utf8_leading_byte_value[c++] = (i != 64) ? (uint8_t)j : 0;
	utf8_leading_byte_value[c] = 0;

	/* unpack the rle'd state table */
	for (c = 0, i = 0, bitsleft = 0, p = 0; c != 256 * 21;) {
		if (bitsleft < 10) {
			p |= ((uint32_t)utf8_state_table_packed[i++] << bitsleft);
			bitsleft += 16;
		}
		bitsleft -= 10;
		val = (p & 0x1f) * UTF8_TABLE_PRECALC_MULTIPLIER; p >>= 5;
		count = (p & 0x1f); p >>= 5;
		count = (count < 21) ? count : (count < 30) ? utf8_state_table_lengths[count-21] : 640;
		while (count--)
			utf8_state_table[c++] = val;
	}
}

/* does this presumed UTF-8 stream have any invalid bytes or invalid codepoints? */
int
utf8_is_valid(const uint8_t *m, size_t len) {
	UTF8_TABLE_TYPE state;
	size_t i;
	for (i = 0, state = 0; i < len; i++)
		state = utf8_state_table[m[i] + (state * UTF8_TABLE_MULTIPLIER)];
	return state == utf8_accept;
}

/* helper to decode a single unicode character from the presumed UTF-8 byte stream, input assumed to have >= 6 bytes in it */
static inline const uint8_t *
utf8_decode_unsafe(const uint8_t *m, uint32_t *c) {
	UTF8_TABLE_TYPE state = utf8_state_table[*m];
	*c = utf8_leading_byte_value[*m++];
	while (state > utf8_fail) {
		state = utf8_state_table[*m + (state * UTF8_TABLE_MULTIPLIER)];
		*c = (*c << 6) | (*m++ & 0x3f);
	}
	if (state != utf8_accept) {
		*c = utf_replacement;
		m -= (state == utf8_fail);
	}
	return m;
}

/* helper to decode a single unicode character from the presumed UTF-8 byte stream, also verifies input length */
static inline const uint8_t *
utf8_decode(const uint8_t *m, const uint8_t *end, uint32_t *c) {
	UTF8_TABLE_TYPE state = utf8_state_table[*m];
	*c = utf8_leading_byte_value[*m++];
	while ((state > utf8_fail) && (m < end)) {
		state = utf8_state_table[*m + (state * UTF8_TABLE_MULTIPLIER)];
		*c = (*c << 6) | (*m++ & 0x3f);
	}
	if (state != utf8_accept) {
		*c = utf_replacement;
		m -= (state == utf8_fail);
	}
	return m;
}

/* helper to encode a UTF-32 character as UTF-16 (no error checking) */
static inline uint16_t *
utf32_to_utf16_unsafe(uint16_t *out, uint32_t c) {
	if (c < 0x10000) {
		out[0] = (uint16_t)c;
		return out + 1;
	} else {
		out[0] = (uint16_t)((0xd800 - (0x10000 >> 10)) + (c >> 10));
		out[1] = (uint16_t)((c & 0x3ff) | 0xdc00);
		return out + 2;
	}
}

/* convert a UTF-8 stream to UTF-16 */
size_t
utf8_to_utf16(const uint8_t *m, size_t mlen, uint16_t *out) {
	const uint8_t *end = m + mlen, *end6 = (mlen >= 6) ? (end - 6) : m;
	uint16_t *start = out;
	uint32_t c;

	while (m < end6) {
		while ((m < end6) && (*m < 0x80))
			*out++ = (uint16_t)*m++;

		while ((m < end6) && (*m >= 0x80)) {
			m = utf8_decode_unsafe(m, &c);
			out = utf32_to_utf16_unsafe(out, c);
		}
	}

	while (m < end) {
		m = utf8_decode(m, end, &c);
		out = utf32_to_utf16_unsafe(out, c);
	}

	return out - start;
}

/* incremental UTF-8 -> UTF-16 decoder */
void
utf8_to_utf16_init(utf8_decode_state *st) {
	st->c = 0;
	st->state = 0;
}

static inline void
utf8_decode_continue_utf16(const uint8_t **m, const uint8_t *end, UTF8_TABLE_TYPE *state, uint32_t *c, uint16_t **out) {
	while ((*state > utf8_fail) && (*m < end)) {
		*state = utf8_state_table[*(*m) + (*state * UTF8_TABLE_MULTIPLIER)];
		*c = (*c << 6) | (*(*m)++ & 0x3f);
	}
	if (*state <= utf8_fail) {
		if (*state != utf8_accept) {
			*c = utf_replacement;
			*m -= (*state == utf8_fail);
		}
		if (*c < 0x10000) {
			*(*out)++ = (uint16_t)*c;
		} else {
			(*out)[0] = (uint16_t)((0xd800 - (0x10000 >> 10)) + (*c >> 10));
			(*out)[1] = (uint16_t)((*c & 0x3ff) | 0xdc00);
			(*out) += 2;
		}
	}
}

void
utf8_to_utf16_continue(utf8_decode_state *st, const uint8_t *m, size_t mlen, size_t *read, uint16_t *out, size_t *written) {
	const uint8_t *m_start = m, *m_end = m_start + mlen;
	uint16_t *out_start = out;
	UTF8_TABLE_TYPE state = (UTF8_TABLE_TYPE)st->state;
	uint32_t c = st->c;

	if (state)
		utf8_decode_continue_utf16(&m, m_end, &state, &c, &out);

	while (m < m_end) {
		while ((m < m_end) && (*m < 0x80))
			*out++ = (uint16_t)*m++;

		while ((m < m_end) && (*m >= 0x80)) {
			state = utf8_state_table[*m];
			c = utf8_leading_byte_value[*m++];
			utf8_decode_continue_utf16(&m, m_end, &state, &c, &out);
		}
	}

	*read = m - m_start;
	*written = out - out_start;
	st->state = (state <= utf8_fail) ? 0 : state;
	st->c = c;
}

void
utf8_to_utf16_finish(utf8_decode_state *st, uint16_t *out, size_t *written) {
	*written = 0;

	if (st->state != utf8_accept) {
		*written = 1;
		*out = utf_replacement;
	}
}


/* convert a UTF-8 stream to UTF-32 */
size_t
utf8_to_utf32(const uint8_t *m, size_t mlen, uint32_t *out) {
	const uint8_t *end = m + mlen, *end6 = (mlen >= 6) ? (end - 6) : m;
	uint32_t *start = out, c;

	while (m < end6) {
		while ((m < end6) && (*m < 0x80))
			*out++ = (uint32_t)*m++;

		while ((m < end6) && (*m >= 0x80)) {
			m = utf8_decode_unsafe(m, &c);
			*out++ = c;
		}
	}

	while (m < end) {
		m = utf8_decode(m, end, &c);
		*out++ = c;
	}

	return out - start;
}

/* incremental UTF-8 -> UTF-32 decoder */
void
utf8_to_utf32_init(utf8_decode_state *st) {
	st->c = 0;
	st->state = 0;
}

static inline void
utf8_decode_continue_utf32(const uint8_t **m, const uint8_t *end, UTF8_TABLE_TYPE *state, uint32_t *c, uint32_t **out) {
	while ((*state > utf8_fail) && (*m < end)) {
		*state = utf8_state_table[*(*m) + (*state * UTF8_TABLE_MULTIPLIER)];
		*c = (*c << 6) | (*(*m)++ & 0x3f);
	}
	if (*state <= utf8_fail) {
		if (*state != utf8_accept) {
			*c = utf_replacement;
			*m -= (*state == utf8_fail);
		}
		*(*out)++ = *c;
	}
}

void
utf8_to_utf32_continue(utf8_decode_state *st, const uint8_t *m, size_t mlen, size_t *read, uint32_t *out, size_t *written) {
	const uint8_t *m_start = m, *m_end = m_start + mlen;
	uint32_t *out_start = out;
	UTF8_TABLE_TYPE state = (UTF8_TABLE_TYPE)st->state;
	uint32_t c = st->c;

	if (state)
		utf8_decode_continue_utf32(&m, m_end, &state, &c, &out);

	while (m < m_end) {
		while ((m < m_end) && (*m < 0x80))
			*out++ = (uint32_t)*m++;

		while ((m < m_end) && (*m >= 0x80)) {
			state = utf8_state_table[*m];
			c = utf8_leading_byte_value[*m++];
			utf8_decode_continue_utf32(&m, m_end, &state, &c, &out);
		}
	}

	*read = m - m_start;
	*written = out - out_start;
	st->state = (state <= utf8_fail) ? 0 : state;
	st->c = c;
}

void
utf8_to_utf32_finish(utf8_decode_state *st, uint32_t *out, size_t *written) {
	*written = 0;

	if (st->state != utf8_accept) {
		*written = 1;
		*out = utf_replacement;
	}
}

