// -*- coding: us-ascii-unix -*-
#include "test-sys/test.hh"
#include "tests/test-util/print-objects.hh"
#include "text/char-constants.hh"
#include "text/text-buffer.hh"

void test_text_buffer(){
  using namespace faint;
  {
    // Test misc TextBuffer functionality

    TextBuffer b;
    EQUAL(b.caret(), 0);

    b.move_down();
    EQUAL(b.caret(), 0);

    b.del();
    EQUAL(b.caret(), 0);

    b.insert(utf8_char("b"));
    EQUAL(b.caret(), 1); // b|

    b.devance();
    EQUAL(b.caret(), 0); // |b

    b.advance();
    EQUAL(b.caret(), 1); // b|

    b.advance();
    EQUAL(b.caret(), 1); // b|

    b.devance();
    EQUAL(b.caret(), 0); // |b

    b.move_down();
    EQUAL(b.caret(), 1); // b|

    b.move_up();
    EQUAL(b.caret(), 0); // |b

    b.insert(utf8_char("a"));
    EQUAL(b.get(), "ab");
    EQUAL(b.caret(), 1) // a|b

      b.move_down();
    EQUAL(b.caret(), 2) // ab|
      b.insert(utf8_char("c"));
    EQUAL(b.get(), "abc");

    // Select entire line by moving up
    b.move_up(true);
    EQUAL(b.caret(), 0); // |abc]
    VERIFY(b.get_sel_range() == CaretRange(0,3));
    EQUAL(b.get_selection(), "abc");

    // Advancing without selection deselects
    b.advance();
    VERIFY(b.get_sel_range() == CaretRange(0,0));
    EQUAL(b.caret(), 1);

    b.advance(true);
    VERIFY(b.get_sel_range() == CaretRange(1,2));

    b.advance(true);
    VERIFY(b.get_sel_range() == CaretRange(1,3));

    b.advance(true);
    VERIFY(b.get_sel_range() == CaretRange(1,3));

    b.insert("ardvark");
    EQUAL(b.get(), "aardvark");
    VERIFY(b.get_sel_range() == CaretRange(0,0));
    EQUAL(b.caret(), 8); // aardvark|

    b.insert(eol);
    b.advance();
    EQUAL(b.caret(), 9); // aardvark^|
    EQUAL(b.get(), "aardvark\n");

    b.devance();
    EQUAL(b.caret(), 8); // aardvark|^

    EQUAL(b.next(eol), 8);
    EQUAL(b.prev(eol), 0); // A bit weird to give 0 instead of npos.
    EQUAL(b.next(snowman), 9); // A bit weird to give size instead of npos.

    b.caret(b.size());
    EQUAL(b.caret(), b.size());

    b.insert("Snaky: ...looks like we're shy one horse.\n");
    b.insert("Harmonica: You brought two too many.\n");
    VERIFY(word_boundaries(10, b) == CaretRange(9,15));

    b.select((word_boundaries(26, b)));
    EQUAL(b.get_selection(), "like");

    b.select(word_boundaries(15, b));
    EQUAL(b.get_selection(), " ");

    b.caret(9);
    b.caret(b.next(eol), true);
    EQUAL(b.get_selection(), "Snaky: ...looks like we're shy one horse.");

    b.del();
    EQUAL(b.caret(), 9);
    VERIFY(b.get_sel_range() == CaretRange(0,0));
    EQUAL(b.at(b.caret()), eol);
    b.del();
    EQUAL(b.caret(), 9);
    EQUAL(b.at(b.caret()), utf8_char("H"));
    b.del_back();
    EQUAL(b.caret(), 8);
    EQUAL(b.at(b.caret()), utf8_char("H"));

    TextBuffer b2(b);
    EQUAL(b.caret(), 8);
    EQUAL(b.at(b.caret()), utf8_char("H"));
    EQUAL(b2.caret(), 8);
    EQUAL(b2.at(b2.caret()), utf8_char("H"));
  }

  {
    // Test "transpose_chars", semi long string

    TextBuffer b("Teh Faint Graphisc\neditor");
    b.caret(2);
    EQUAL(b.caret(), 2);
    transpose_chars(b);
    EQUAL(b.get(), "The Faint Graphisc\neditor");
    EQUAL(b.caret(), 3);

    b.caret(17);
    transpose_chars(b);
    EQUAL(b.get(), "The Faint Graphics\neditor");
    EQUAL(b.caret(), 18);

    b.caret(24);
    transpose_chars(b);
    EQUAL(b.get(), "The Faint Graphics\neditro");
    EQUAL(b.caret(), 25);

    transpose_chars(b);
    EQUAL(b.get(), "The Faint Graphics\neditor");
    EQUAL(b.caret(), 25);
  }

  {
    // Test "transpose_chars", one character string
    TextBuffer b("A");
    b.caret(0);
    transpose_chars(b);
    EQUAL(b.caret(), 0);
    b.caret(1);
    transpose_chars(b);
    EQUAL(b.caret(), 1);
    EQUAL(b.size(), 1);
    EQUAL(b.at(0), utf8_char("A"));
  }

  {
    // Test "transpose_chars", empty string
    TextBuffer b;
    EQUAL(b.caret(), 0);
    transpose_chars(b);
    EQUAL(b.caret(), 0);
    EQUAL(b.size(), 0);
  }

  {
    // Test TextBuffer-comment claims
    TextBuffer b("Hello world");

    // "The caret position is zero based"
    b.caret(0);
    b.del();
    EQUAL(b.get(), "ello world");
    b.insert("H");
    EQUAL(b.get(), "Hello world");
    b.caret(1);
    b.del_back();
    EQUAL(b.get(), "ello world");
    EQUAL(b.caret(), 0);
    b.insert("H");
    b.caret(b.size());
    EQUAL(b.caret(), 11);

    // "The selection is similar to having two carets"
    b.caret(1);
    b.caret(4,true);
    EQUAL(b.get_selection(), "ell");
    EQUAL(b.get_sel_range().from, 1);
    EQUAL(b.get_sel_range().to, 4);

    b.select_none();
    EQUAL(b.get_selection(), "");
    EQUAL(b.caret(), 4);
    b.caret(1, true);
    EQUAL(b.get_selection(), "ell");
    EQUAL(b.get_sel_range().from, 1);
    EQUAL(b.get_sel_range().to, 4);
  }
}
