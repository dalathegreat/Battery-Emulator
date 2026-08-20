#ifndef _ESCAPE_H_
#define _ESCAPE_H_

#include <WString.h>

/* Output escaping, kept in one place deliberately.
 *
 * There used to be two copies: this one, and a private tr_html_escape() inside
 * the i18n runtime that existed only so i18n would not have to depend on the
 * webserver. They agreed by inspection, nothing enforced it, and only one of
 * them was covered by the differential test - so the escaper that TR()
 * actually used was the untested one. Escaping lives here, below both, and is
 * tested once.
 *
 * Correctness of an escaper is a property of the SINK, not of the string, so
 * pick by where the value lands:
 *   html_escape()      element text and quoted attribute values
 *   js_string_escape() inside a JavaScript string literal
 * Neither is right for a text/plain body, a JSON value or an MQTT payload -
 * those take the unescaped text.
 *
 * tools/test_escaping_differential.py checks these against real parsers rather
 * than against a list of characters: escape, embed, parse, and assert the
 * value survived unchanged. Add a sink there before adding one here.
 */

// & < > " ' -> entities. UTF-8 passes through untouched.
String html_escape(const String& var);

/* Escape for a single-, double- or backtick-quoted JavaScript string literal
 * inside a <script> element. Covers the quote characters and backslash; also
 * backtick and $, because `${...}` in a template literal does not merely end
 * the literal, it evaluates; also "</", because script content is raw text to
 * the HTML parser, so a "</script>" inside a literal still ends the element.
 * Line terminators become spaces - they would end the literal, and a UI label
 * has no business containing one. */
String js_string_escape(const String& var);

#endif
