// -*- coding: us-ascii-unix -*-
#include "test-sys/test.hh"
#include "text/auto-complete.hh"
#include "text/char-constants.hh"

void test_auto_complete(){
  using namespace faint;

  const auto euroStr = utf8_string(1, euro_sign);
  const auto snowmanStr = utf8_string(1, snowman);
  const auto sqSstr = utf8_string(1, superscript_two);

  AutoComplete ac({euroStr,
        "a",
        "ab",
        "aaa",
        "abc",
        "abb",
        "ab1",
        "ab2",
        "qwerty",
        snowmanStr + snowmanStr,
        snowmanStr + sqSstr + euroStr});

  {
    // Test "AutoComplete"
    Words w = ac.match("a");
    ASSERT(w.size() == 7);
    NOT(w.empty());
    VERIFY(w.get(0) == "a");
    VERIFY(w.get(1) == "aaa");
    VERIFY(w.get(2) == "ab");
    VERIFY(w.get(3) == "ab1");
    VERIFY(w.get(4) == "ab2");
    VERIFY(w.get(5) == "abb");
    VERIFY(w.get(6) == "abc");

    w = ac.match("b");
    VERIFY(w.size() == 0);
    VERIFY(w.empty());

    w = ac.match("ab");
    ASSERT(w.size() == 5);
    VERIFY(w.get(0) == "ab");
    VERIFY(w.get(1) == "ab1");
    VERIFY(w.get(2) == "ab2");
    VERIFY(w.get(3) == "abb");
    VERIFY(w.get(4) == "abc");

    w = ac.match("ab1");
    ASSERT(w.size() == 1);
    VERIFY(w.get(0) == "ab1");

    w = ac.match("abc");
    ASSERT(w.size() == 1);
    VERIFY(w.get(0) == "abc");

    w = ac.match("abcd");
    VERIFY(w.empty());

    w = ac.match("q");
    ASSERT(w.size() == 1);
    VERIFY(w.get(0) == "qwerty");

    w = ac.match(snowmanStr);
    ASSERT(w.size() == 2);
    VERIFY(w.get(0) == snowmanStr + sqSstr + euroStr);
    VERIFY(w.get(1) == snowmanStr + snowmanStr);

    w = ac.match(euroStr);
    ASSERT(w.size() == 1);
    VERIFY(w.get(0) == euroStr);
  }

  {
    // Test "AutoCompleteState"
    AutoCompleteState acs(ac);
    VERIFY(acs.Complete("a") == "a");
    ASSERT(!acs.Empty());
    VERIFY(acs.Next() == "aaa");
    VERIFY(acs.Next() == "ab");
    VERIFY(acs.Next() == "ab1");
    VERIFY(acs.Next() == "ab2");
    VERIFY(acs.Next() == "abb");
    VERIFY(acs.Next() == "abc");
    VERIFY(acs.Next() == "a");
    VERIFY(acs.Prev() == "abc");
    VERIFY(acs.Prev() == "abb");

    VERIFY(acs.Complete("ab1") == "ab1");
    ASSERT(!acs.Empty());
    VERIFY(acs.Next() == "ab1");
    VERIFY(acs.Next() == "ab1");
    VERIFY(acs.Prev() == "ab1");
    VERIFY(acs.Prev() == "ab1");

    acs.Forget();
    VERIFY(acs.Empty());

    VERIFY(acs.Complete("q") == "qwerty");
    ASSERT(!acs.Empty());
    VERIFY(acs.Next() == "qwerty");

    VERIFY(acs.Complete(snowmanStr) == snowmanStr + sqSstr + euroStr);
    ASSERT(!acs.Empty());
    VERIFY(acs.Next() == snowmanStr + snowmanStr);
    VERIFY(acs.Next() == snowmanStr + sqSstr + euroStr);
  }
}
