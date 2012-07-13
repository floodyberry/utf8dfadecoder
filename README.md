This is a UTF-8 [DFA](http://en.wikipedia.org/wiki/Deterministic_finite_automaton) decoder, based on Björn Höhrmann's [Flexible and Economical UTF-8 Decoder](http://bjoern.hoehrmann.de/utf-8/decoder/dfa/). I really liked how simple it was, but I wanted a few changes:

1. It allowed some wiggly non-characters through (0xfdd0-0xfdef, 0x??fffe/0x??ffff)
2. It didn't handle 5 or 6 byte UTF-8 sequences, i.e. fully consume them and emit a replacement character
3. It didn't distinguish between an invalid byte in a UTF-8 stream, and a validly encoded yet illegal value, e.g. overlong encodings. This meant the decoder had no way to know if it should back up because it encountered an illegal byte.

The somewhat arbitrary goal was a decoder that emitted 1 replacement character for each unexpected byte, 1 replacement character for each unfinished UTF-8 sequence up to the point where the sequence was still legal, and 1 replacement character for each valid UTF-8 sequence that represents a non-valid codepoint or overlong encoding. 

I still use the basic form he presented, but generated expanded state tables to handle the new requirements, and slightly modified the innerloop to only require one state lookup at the expense of a larger tables (256 + 108) bytes vs (256 + 5376) bytes.