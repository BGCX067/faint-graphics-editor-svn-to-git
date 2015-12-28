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

#ifndef FAINT_SPLIT_STRING_HH
#define FAINT_SPLIT_STRING_HH
#include <string>
#include <vector>
#include "text/utf8-string.hh"
#include "util/template-fwd.hh"

namespace faint{

class Settings;
class FaintDC;

class line_t{
public:
  static line_t soft_break(coord, const utf8_string&);
  static line_t hard_break(coord, const utf8_string&);
  bool hardBreak;
  utf8_string text;
  coord width;
private:
  line_t(bool, coord, const utf8_string&);
};
typedef std::vector<line_t> text_lines_t;

typedef Optional<coord> max_width_t;

// Split the string at line breaks, and within lines that are longer
// than max-width pixels, if specified.
text_lines_t split_str( FaintDC&, const utf8_string&, const Settings&, const max_width_t& );

} // namespace

#endif
