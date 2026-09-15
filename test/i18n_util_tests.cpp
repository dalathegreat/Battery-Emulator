#include <gtest/gtest.h>

#include "../Software/src/devboard/i18n/i18n_util.h"

// Host tests for the i18n stack's pure helpers: catalog filename validation
// and the installed-language listing.

TEST(I18nUtilTest, ValidFilenames) {
  EXPECT_TRUE(is_valid_i18n_filename("sv.json.gz"));
  EXPECT_TRUE(is_valid_i18n_filename("uk.json.gz"));
  EXPECT_TRUE(is_valid_i18n_filename("pt-BR.json.gz"));
  EXPECT_TRUE(is_valid_i18n_filename("sv.blp"));
  EXPECT_TRUE(is_valid_i18n_filename("pt-BR.blp"));
}

TEST(I18nUtilTest, InvalidFilenames) {
  EXPECT_FALSE(is_valid_i18n_filename(""));
  EXPECT_FALSE(is_valid_i18n_filename("s.json.gz"));
  EXPECT_FALSE(is_valid_i18n_filename("SV.json.gz"));
  EXPECT_FALSE(is_valid_i18n_filename("sve.json.gz"));
  EXPECT_FALSE(is_valid_i18n_filename("sv.json"));
  EXPECT_FALSE(is_valid_i18n_filename("sv-br.json.gz"));
  EXPECT_FALSE(is_valid_i18n_filename("sv.json.gz.bak"));
  EXPECT_FALSE(is_valid_i18n_filename("../sv.json.gz"));
  EXPECT_FALSE(is_valid_i18n_filename("sv/../x.json.gz"));
}

TEST(I18nUtilTest, LanguageFromFilename) {
  EXPECT_STREQ(i18n_language_from_filename("sv.json.gz").c_str(), "sv");
  EXPECT_STREQ(i18n_language_from_filename("pt-BR.blp").c_str(), "pt-BR");
  EXPECT_STREQ(i18n_language_from_filename("nonsense").c_str(), "");
}

TEST(I18nUtilTest, ListJsonDeduplicatesAcrossFormats) {
  std::vector<String> files = {"sv.json.gz", "sv.blp", "fi.json.gz", "junk.txt"};
  String json = i18n_list_json(files, "sv");
  EXPECT_STREQ(json.c_str(), "{\"active\":\"sv\",\"languages\":[\"sv\",\"fi\"]}");
}

TEST(I18nUtilTest, ListJsonEmptyStorage) {
  std::vector<String> files;
  String json = i18n_list_json(files, "");
  EXPECT_STREQ(json.c_str(), "{\"active\":\"\",\"languages\":[]}");
}

TEST(I18nUtilTest, ValidLanguageCodes) {
  EXPECT_TRUE(is_valid_language_code("sv"));
  EXPECT_TRUE(is_valid_language_code("pt-BR"));
  EXPECT_FALSE(is_valid_language_code(""));
  EXPECT_FALSE(is_valid_language_code("s"));
  EXPECT_FALSE(is_valid_language_code("SV"));
  EXPECT_FALSE(is_valid_language_code("sve"));
  EXPECT_FALSE(is_valid_language_code("pt-br"));
  EXPECT_FALSE(is_valid_language_code("pt_BR"));
  EXPECT_FALSE(is_valid_language_code("sv\"x"));
}

// A stale or hand-written NVS value must not be able to break out of the
// JSON string literal it is interpolated into.
TEST(I18nUtilTest, ListJsonRejectsMalformedActiveLanguage) {
  std::vector<String> files = {"sv.json.gz"};
  EXPECT_STREQ(i18n_list_json(files, "\",\"x\":\"").c_str(), "{\"active\":\"\",\"languages\":[\"sv\"]}");
  EXPECT_STREQ(i18n_list_json(files, "</script>").c_str(), "{\"active\":\"\",\"languages\":[\"sv\"]}");
  EXPECT_STREQ(i18n_list_json(files, "sv").c_str(), "{\"active\":\"sv\",\"languages\":[\"sv\"]}");
}
