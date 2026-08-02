/* Emits the escaper output for the differential test in
 * tools/test_escaping_differential.py.
 *
 * The point is to exercise the REAL escapers rather than a reimplementation:
 * the driver feeds strings in, embeds the results in actual HTML and
 * JavaScript, parses them with a real parser, and checks the value that
 * comes out the other side is the value that went in. That detects a missing
 * escape for any character, including ones nobody thought to add.
 *
 * Protocol (deliberately trivial, and binary-safe): stdin is
 *   <decimal byte length> LF <that many raw bytes>
 * repeated. For each input, stdout gets
 *   H <len> LF <html_escape bytes> LF
 *   J <len> LF <js_string_escape bytes> LF
 */
#include <cstdio>
#include <string>
#include <vector>
#include "../Software/src/devboard/utils/escape.h"

static void emit(char tag, const String& value) {
  std::string s(value.c_str());
  printf("%c %zu\n", tag, s.size());
  fwrite(s.data(), 1, s.size(), stdout);
  putchar('\n');
}

int main() {
  for (;;) {
    long len = -1;
    if (scanf("%ld", &len) != 1) {
      break;
    }
    getchar();  // the LF after the length
    std::vector<char> buf((size_t)len + 1, 0);
    if (len > 0 && fread(buf.data(), 1, (size_t)len, stdin) != (size_t)len) {
      break;
    }
    String input(buf.data());
    emit('H', html_escape(input));
    emit('J', js_string_escape(input));
    fflush(stdout);
  }
  return 0;
}
