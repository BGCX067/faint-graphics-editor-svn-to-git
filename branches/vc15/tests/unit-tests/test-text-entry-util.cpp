// -*- coding: us-ascii-unix -*-
#include "test-sys/test.hh"
#include "text/char-constants.hh"
#include "text/text-buffer.hh"
#include "util/text-entry-util.hh"
#include "util-wx/key-codes.hh"

void test_text_entry_util(){
  using namespace faint;
  TextBuffer text("Hello world");
  text.caret(text.size());

  // Enter "a"
  VERIFY(handle_key_press(KeyPress(None, key::A, utf8_char("a")), text));
  VERIFY(text.get()[11] == utf8_char("a"));

  // Invalid character (null)
  VERIFY(!handle_key_press(KeyPress(None, key::A, utf8_null), text));
  VERIFY(text.get()[11] == utf8_char("a"));

  // Invalid character (replacement)
  VERIFY(!handle_key_press(KeyPress(None, key::A, replacement_character), text));
  VERIFY(text.get()[11] == utf8_char("a"));

  // Navigation
  VERIFY(handle_key_press(KeyPress(None, key::up, utf8_null), text));
  VERIFY(text.caret() == 0);
  VERIFY(text.get()[11] == utf8_char("a"));

  VERIFY(handle_key_press(KeyPress(None, key::down, utf8_null), text));
  VERIFY(text.caret() == 12);
  VERIFY(text.get()[11] == utf8_char("a"));

  // Selection
  VERIFY(handle_key_press(KeyPress(Shift, key::left, utf8_null), text));
  VERIFY(text.caret() == 11);
  VERIFY(text.get_sel_range().from == 11);
  VERIFY(text.get_sel_range().to == 12);

  // Transpose
  VERIFY(text.get() == "Hello worlda");
  text.caret(5);
  VERIFY(handle_key_press(KeyPress(Ctrl, key::T, utf8_null), text));
  VERIFY(text.get() == "Hell oworlda");
}
