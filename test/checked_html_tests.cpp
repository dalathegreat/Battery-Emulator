#include <WString.h>
#include <gtest/gtest.h>

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
