// -*- coding: us-ascii-unix -*-
// Copyright 2012 Lukas Kemmer
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

#include <algorithm>
#include <string>
#include "geo/size.hh"
#include "rendering/faint-dc.hh"
#include "text/char-constants.hh"
#include "text/split-string.hh"
#include "util/optional.hh"

namespace faint{

inline coord getw(FaintDC& dc, const utf8_string& str, const Settings& settings){
  Size sz(dc.TextSize(str, settings));
  return sz.w;
}

#ifdef DEBUG_SPLIT_STR
const utf8_char wordBrk('>');
const utf8_char& hardBrk = pilcrow_sign;
const utf8_char& softBrk = downwards_arrow_with_tip_leftwards;
#else
const utf8_char& wordBrk = space;
const utf8_char& hardBrk = space;
const utf8_char& softBrk = space;
#endif

line_t line_t::soft_break(coord w, const utf8_string& str){
  return line_t(false, w, str);
}

line_t line_t::hard_break(coord w, const utf8_string& str){
  return line_t(true, w, str);
}

line_t::line_t(bool in_hardBreak, coord in_width, const utf8_string& in_text)
  : hardBreak(in_hardBreak),
    text(in_text),
    width(in_width)
{}

static utf8_string split_word(FaintDC& dc, const utf8_string& string, text_lines_t& result, const Settings& settings){
  // wxArrayInt widths;
  // GetPartialTextExtents broken for gcdc?
  // (All fields set to 0)
  /* bool ok = dc.GetPartialTextExtents(string, widths);
  assert(widths.size() > 0);
  assert(ok);*/

  // <- The old Commented stuff apparently used wxGCDC, and did not
  // work. We use a FaintDC now, so something similar to the above
  // should be done for that.  Also some hyphenization could be
  // cool. BUT: Just break the word in two for now!

  const utf8_string half(string.substr(0, string.size() / 2) + wordBrk);
  const coord width = getw(dc, half, settings);
  result.push_back(line_t::soft_break(width, half));
  return string.substr(string.size() / 2, string.size() - string.size() / 2);
}

// Split a line at suitable positions to make it shorter than
// maxWidth. The line should not contain embedded line breaks.
void split_line(FaintDC& dc, const utf8_string& string, coord maxWidth, text_lines_t& result, const Settings& settings){
  size_t wordStart = 0;
  size_t wordEnd = 0;

  utf8_string line;
  do {
    wordEnd = string.find(utf8_char(" "), wordStart);
    if (wordEnd == std::string::npos){
      wordEnd = string.size();
    }
    utf8_string word = string.substr(wordStart, wordEnd - wordStart );
    const coord width = getw(dc, line + space + word, settings);
    if (!line.empty() && width > maxWidth){
      result.push_back(line_t::soft_break(width, line + softBrk));
      line.clear();
    }

    if (getw(dc, word, settings) > maxWidth) {
      word = split_word(dc, word, result, settings);
    }

    if (!line.empty()){
      line += space;
    }

    line += word;
    wordStart = wordEnd + 1;
  } while (wordEnd != string.size());

  if (line.size() > 1){
    const utf8_string last(line + softBrk);
    const coord width = getw(dc, last, settings);
    result.push_back(line_t::soft_break(width, last));
  }
}

text_lines_t split_str(FaintDC& dc, const utf8_string& string, const Settings& settings, const max_width_t& maxWidth){
  size_t lineEnd = 0;
  size_t lineStart = 0;

  text_lines_t result;
  do {
    lineEnd = string.find(eol, lineStart);
    bool softBreak = lineEnd == std::string::npos;
    if (softBreak){
      lineEnd = string.size();
    }

    const coord width = getw(dc, string.substr(lineStart, lineEnd - lineStart), settings);
    if (maxWidth.NotSet() || width < maxWidth.Get()){
      if (softBreak){
        result.push_back(line_t::soft_break(width, string.substr(lineStart, lineEnd - lineStart) + hardBrk));
      }
      else {
        result.push_back(line_t::hard_break(width, string.substr(lineStart, lineEnd - lineStart) + hardBrk));
      }
    }
    else {
      split_line(dc, string.substr(lineStart, lineEnd - lineStart), maxWidth.Get(), result, settings);
    }

    lineStart = lineEnd + 1;

  } while (lineEnd != string.size());
  return result;
}

} // namespace
