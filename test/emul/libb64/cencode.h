#ifndef BASE64_CENCODE_H
#define BASE64_CENCODE_H

#define base64_encode_expected_len(n) ((((4 * n) / 3) + 3) & ~3)

int base64_encode_chars(const char* plaintext_in, int length_in, char* code_out);

#endif
