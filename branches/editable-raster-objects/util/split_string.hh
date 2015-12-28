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

#ifndef FAINT_SPLIT_STR_HH
#define FAINT_SPLIT_STR_HH
#include <string>
#include <vector>
#include "util/char.hh"

class Settings;
class FaintDC;

typedef std::vector<std::pair<bool, faint::utf8_string> > text_lines_t;

typedef Optional<faint::coord> max_width_t;

// Split the string at line breaks, and within lines that are longer
// than max-width pixels, if specified.
text_lines_t split_str( FaintDC&, const faint::utf8_string&, const Settings&, const max_width_t& );

#endif
