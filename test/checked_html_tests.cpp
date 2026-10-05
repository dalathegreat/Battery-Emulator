#include <WString.h>
#include <gtest/gtest.h>

#include <type_traits>
#include <utility>

#include "../Software/src/devboard/webserver/checked_html.h"

TEST(CheckedHtmlTest, EmptyStringIsValid) {
  const String empty;
  EXPECT_TRUE(static_cast<bool>(empty));
  EXPECT_TRUE(empty.isEmpty());

  CheckedHtml html;
  html += empty;
  EXPECT_TRUE(html.good());
  EXPECT_TRUE(html.take().isEmpty());
}

TEST(CheckedHtmlTest, AppendsTextAndTransfersResult) {
  CheckedHtml html;
  ASSERT_TRUE(html.reserve(32));
  html += "<p>";
  html += String("Battery 2");
  html += "</p>";
  EXPECT_TRUE(html.good());
  EXPECT_STREQ(html.take().c_str(), "<p>Battery 2</p>");
}

TEST(CheckedHtmlTest, FailedAppendDiscardsPartialOutput) {
  CheckedHtml html;
  html += "partial";
  html += static_cast<const char*>(nullptr);
  html += "later";
  EXPECT_FALSE(html.good());
  EXPECT_FALSE(html.reserve(64));
  EXPECT_TRUE(html.take().isEmpty());
}

TEST(CheckedHtmlTest, AppendsASingleChar) {
  CheckedHtml html;
  ASSERT_TRUE(html.reserve(32));
  html += "a";
  html += ' ';
  html += String("b");
  EXPECT_TRUE(html.good());
  EXPECT_STREQ(html.take().c_str(), "a b");
}

/* Declaring any operator+= on CheckedHtml hides every one String defines, so the accepted forms
   have to be named one by one - and the numeric ones deleted rather than merely left out. Without
   the deletions `html += 5` would convert to char and silently append a control character. These
   assertions fail at compile time if a numeric overload ever creeps back in. */
template <typename T, typename = void>
struct is_appendable : std::false_type {};

template <typename T>
struct is_appendable<T, decltype(void(std::declval<CheckedHtml&>() += std::declval<T>()))> : std::true_type {};

TEST(CheckedHtmlTest, OnlyTextFormsAreAppendable) {
  static_assert(is_appendable<const char*>::value, "const char* must append");
  static_assert(is_appendable<String>::value, "String must append");
  static_assert(is_appendable<char>::value, "char must append");

  static_assert(!is_appendable<int>::value, "numbers must be converted explicitly");
  static_assert(!is_appendable<unsigned int>::value, "numbers must be converted explicitly");
  static_assert(!is_appendable<long>::value, "numbers must be converted explicitly");
  static_assert(!is_appendable<float>::value, "numbers must be converted explicitly");
  static_assert(!is_appendable<double>::value, "numbers must be converted explicitly");
  static_assert(!is_appendable<unsigned char>::value, "numbers must be converted explicitly");
  SUCCEED();
}
