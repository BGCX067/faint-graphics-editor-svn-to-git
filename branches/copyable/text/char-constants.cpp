// -*- coding: us-ascii-unix -*-
// Copyright 2014 Lukas Kemmer
//
// Licensed under the Apache License, Version 2.0 (the "License"); you
// may not use this file except in compliance with the License. You
// may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.

#include "text/char-constants.hh"
#include "text/utf8-string.hh"

namespace faint{
  const utf8_char comma(0x2c);
  const utf8_char downwards_arrow_with_tip_leftwards(0x21b2);
  const utf8_char eol(0x0a);
  const utf8_char euro_sign(0x20ac);
  const utf8_char exclamation_mark(0x21);
  const utf8_char full_stop(0x2e);
  const utf8_char hyphen(0x2D);
  const utf8_char left_parenthesis(0x28);
  const utf8_char middle_dot(0x00b7);
  const utf8_char no_break_space(0xc2a0);
  const utf8_char pilcrow_sign(0x00b6);
  const utf8_char question_mark(0x3f);
  const utf8_char right_parenthesis(0x29);
  const utf8_char snowman(0x2603);
  const utf8_char space(0x20);
  const utf8_char tab(0x9);

bool is_punctuation( const utf8_char& ch ){
  return ch == faint::comma ||
    ch == faint::exclamation_mark ||
    ch == faint::full_stop ||
    ch == faint::question_mark;
}

} // namespace
