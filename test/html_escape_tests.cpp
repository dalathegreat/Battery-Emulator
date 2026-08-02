#include <gtest/gtest.h>

#include "../Software/src/devboard/webserver/html_escape.h"

// Catalog text reaching a JS string literal must not be able to close the
// literal, the statement, or the <script> element. This is the shape that
// broke: keys whose value was an entire alert('...') statement.
TEST(HtmlEscapeTest, JsEscapeContainsInjectionAttempts) {
  EXPECT_EQ(js_string_escape(String("l'accu")), String("l\\'accu"));
  EXPECT_EQ(js_string_escape(String("a\\b")), String("a\\\\b"));
  EXPECT_EQ(js_string_escape(String("x');alert(1);//")), String("x\\');alert(1);//"))
      << "Quote escaped, so the injected call stays inside the literal";
  EXPECT_EQ(js_string_escape(String("</script><img onerror=alert(1)>")), String("<\\/script><img onerror=alert(1)>"))
      << "Script content is raw text to the HTML parser: </ must be broken";
  EXPECT_EQ(js_string_escape(String("a\nb\rc")), String("a b c")) << "Line terminators would end the literal";
}

TEST(HtmlEscapeTest, HtmlEscapeCoversMarkupMetacharacters) {
  EXPECT_EQ(html_escape(String("U<20V & rising")), String("U&lt;20V &amp; rising"));
  EXPECT_EQ(html_escape(String("<img onerror=alert(1)>")), String("&lt;img onerror=alert(1)&gt;"));
  EXPECT_EQ(html_escape(String("l'accu \"x\"")), String("l&#39;accu &quot;x&quot;"));
  EXPECT_EQ(html_escape(String("Överspänning")), String("Överspänning")) << "UTF-8 passes through";
}

/* Template literals were the blocker: the cell monitor built two of them out
 * of TR(), and the HTML escaper leaves ` and $ alone, so a translation could
 * close the literal and - via ${...} - execute. Escaping them in the escaper
 * rather than at the call sites is what makes the next one safe by default. */
TEST(HtmlEscapeTest, JsEscapeNeutralisesTemplateLiterals) {
  EXPECT_EQ(js_string_escape(String("a`b")), String("a\\`b"));
  EXPECT_EQ(js_string_escape(String("cost $5")), String("cost \\$5"));
  EXPECT_EQ(js_string_escape(String("${alert(1)}")), String("\\${alert(1)}"))
      << "The $ is escaped, so ${...} cannot start an interpolation";
  EXPECT_EQ(js_string_escape(String("x`;alert(1);`")), String("x\\`;alert(1);\\`"));
  EXPECT_EQ(js_string_escape(String("say \"hi\"")), String("say \\\"hi\\\""))
      << "Double quotes too, so the escaper is safe for all three literal styles";
}
