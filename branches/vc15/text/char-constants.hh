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

#ifndef FAINT_CHAR_CONSTANTS_HH
#define FAINT_CHAR_CONSTANTS_HH
#include "text/utf8-char.hh"

namespace faint{
extern const utf8_char utf8_null;
extern const utf8_char backslash; // aka "reverse solidus"
extern const utf8_char bullet;
extern const utf8_char comma;
extern const utf8_char degree_sign;
extern const utf8_char double_prime; // aka "inch symbol"
extern const utf8_char downwards_arrow_with_tip_leftwards;
extern const utf8_char eol;
extern const utf8_char euro_sign;
extern const utf8_char exclamation_mark;
extern const utf8_char full_stop;
extern const utf8_char greek_capital_letter_delta;
extern const utf8_char greek_small_letter_pi;
extern const utf8_char hyphen;
extern const utf8_char left_parenthesis;
extern const utf8_char no_break_space;
extern const utf8_char period;
extern const utf8_char pilcrow_sign;
extern const utf8_char prime;
extern const utf8_char replacement_character;
extern const utf8_char superscript_two;
extern const utf8_char question_mark;
extern const utf8_char right_parenthesis;
extern const utf8_char shown_eol;
extern const utf8_char snowman;
extern const utf8_char space;
extern const utf8_char tab;
extern const utf8_char triple_prime;

bool is_punctuation(const utf8_char&);

} // namespace

#endif
