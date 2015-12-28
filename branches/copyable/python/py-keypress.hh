// -*- coding: us-ascii-unix -*-
// Copyright 2013 Lukas Kemmer
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

#ifndef FAINT_PY_KEYPRESS_HH
#define FAINT_PY_KEYPRESS_HH
#include <vector>
#include "text/utf8-string.hh"
#include "util/keycode.hh"

namespace faint{
struct BindInfo{
  BindInfo( const KeyPress& in_key, const faint::utf8_string& in_function, const faint::utf8_string& in_docs )
    : key(in_key),
      function(in_function),
      docs(in_docs)
  {}
  KeyPress key;
  faint::utf8_string function;
  faint::utf8_string docs;
};

void python_keypress( const KeyPress& );
std::vector<BindInfo> list_binds();
}

#endif
