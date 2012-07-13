#ifndef UTF8DECODER_H
#define UTF8DECODER_H

#if defined(_MSC_VER)
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#define inline __forceinline
#else
#include <stdint.h>
#endif

#include <stddef.h>

#define utf_replacement 0xfffd

typedef struct utf8_decode_state_t {
	uint32_t state, c;
} utf8_decode_state;

void utf8_unpack_tables(void);

int utf8_is_valid(const uint8_t *m, size_t len);

/* UTF-16 */
size_t utf8_to_utf16(const uint8_t *m, size_t mlen, uint16_t *out);

void utf8_to_utf16_init(utf8_decode_state *st);
void utf8_to_utf16_continue(utf8_decode_state *st, const uint8_t *m, size_t mlen, size_t *read, uint16_t *out, size_t *written);
void utf8_to_utf16_finish(utf8_decode_state *st, uint16_t *out, size_t *written);

/* UTF-32 */
size_t utf8_to_utf32(const uint8_t *m, size_t mlen, uint32_t *out);

void utf8_to_utf32_init(utf8_decode_state *st);
void utf8_to_utf32_continue(utf8_decode_state *st, const uint8_t *m, size_t mlen, size_t *read, uint32_t *out, size_t *written);
void utf8_to_utf32_finish(utf8_decode_state *st, uint32_t *out, size_t *written);

/* largest size of the resulting string from fromchar -> tochar */
inline size_t max_output_utf8_to_utf16_characters(size_t len) { return len; }
inline size_t max_output_utf8_to_utf16_bytes(size_t len) { return max_output_utf8_to_utf16_characters(len) * sizeof(uint16_t); }
inline size_t max_output_utf8_to_utf32_characters(size_t len) { return len; }
inline size_t max_output_utf8_to_utf32_bytes(size_t len) { return max_output_utf8_to_utf32_characters(len) * sizeof(uint32_t); }

#endif /* UTF8DECODER_H */

