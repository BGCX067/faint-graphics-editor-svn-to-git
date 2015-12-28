// -*- coding: us-ascii-unix -*-
#include "test-sys/test.hh"
#include "tests/test-util/print-objects.hh"
#include "text/string-util.hh"
#include "text/char-constants.hh"

void test_utf8_string(){
  using namespace faint;
  VERIFY(utf8_string().empty());
  EQUAL(utf8_string(10, utf8_char("A")).size(), 10);
  EQUAL(utf8_string(10, utf8_char("A")).bytes(), 10);
  VERIFY(is_ascii(utf8_string(10, utf8_char("A"))));

  // A string with all ascii code points
  utf8_string s;
  for (unsigned int i = 0; i != 128; i++){
    s += utf8_char(i);
  }

  VERIFY(is_ascii(s));
  EQUAL(s.size(), 128);
  EQUAL(s.bytes(), s.size());
  EQUAL(s[0], utf8_char(0));
  EQUAL(s[127], utf8_char(127));
  EQUAL(s.find(snowman), utf8_string::npos);

  size_t preBytes = s.bytes();
  EQUAL(snowman.bytes(), 3);
  s += snowman;
  EQUAL(s.bytes(), preBytes + 3);
  EQUAL(s[128], snowman);

  EQUAL(s.find(snowman, 0), 128);
  EQUAL(s.rfind(snowman), 128);

  NOT(is_ascii(s));
  auto s2 = s + s;
  EQUAL(s2.size(), s.size() * 2);
  EQUAL(s2.bytes(), s.bytes() * 2);
  EQUAL(s2.substr(0, s.size()), s);
  s2 += s;

  utf8_string s3(s2);
  EQUAL(s2, s3);

  s2.clear();
  VERIFY(s2.empty());
  EQUAL(s2.size(), 0);
  EQUAL(s2.bytes(), 0);

  utf8_string expr("Hello\\perimeter(rect1, mm)");
  EQUAL(expr.find(utf8_char("\\")), 5);
  EQUAL(expr.find(backslash), 5);

  // Ascii
  EQUAL(utf8_char("A").codepoint(), 0x41);
  EQUAL(utf8_char("B").codepoint(), 0x42);
  EQUAL(utf8_char("a").codepoint(), 0x61);
  EQUAL(utf8_char("b").codepoint(), 0x62);

  // One-byte, first code point
  EQUAL(utf8_char(0x0000).codepoint(), 0x0000);
  EQUAL(utf8_char(0x0000).bytes(), 1);

  // One-byte, last code point
  EQUAL(utf8_char(0x7f).codepoint(), 0x7f);
  EQUAL(utf8_char(0x7f).bytes(), 1);

  // Two-byte, first code point
  EQUAL(utf8_char(0x80).bytes(), 2);
  EQUAL(utf8_char(0x80).codepoint(), 0x80);

  // Two-byte, last code point
  EQUAL(utf8_char(0x7ff).bytes(), 2);
  EQUAL(utf8_char(0x7ff).codepoint(), 0x7ff);

  // Three-byte, first code point
  EQUAL(utf8_char(0x800).bytes(), 3);
  EQUAL(utf8_char(0x800).codepoint(), 0x800);

  // Three-byte, last code point
  EQUAL(utf8_char(0xffff).bytes(), 3);
  EQUAL(utf8_char(0xffff).codepoint(), 0xffff);

  // Four-byte, first code point
  EQUAL(utf8_char(0x10000).bytes(), 4);
  EQUAL(utf8_char(0x10000).codepoint(), 0x10000);

  // Four-byte, last code point
  EQUAL(utf8_char(0x1fffff).bytes(), 4);
  EQUAL(utf8_char(0x1fffff).codepoint(), 0x1fffff);

  // Old favorite
  EQUAL(snowman.codepoint(), 0x2603);

  EQUAL(utf8_char("0").bytes(), 1);
  EQUAL(utf8_char("0").codepoint(), 0x30);
  EQUAL(utf8_char("1").bytes(), 1);
  EQUAL(utf8_char("1").codepoint(), 0x31);

  EQUAL(utf8_char("9").bytes(), 1);
  EQUAL(utf8_char("9").codepoint(), 0x39);
  VERIFY(ends("hello world", with("world")));
  VERIFY(!ends("hello morld", with("world")));
  VERIFY(!ends("orld", with("world")));
  VERIFY(ends("world", with("world")));
}
