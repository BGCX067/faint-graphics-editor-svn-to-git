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

#include "text/string-util.hh"

namespace faint{

bool ends(const utf8_string& s, const with_t& withSubstr){
  const auto& substr(withSubstr.Get());
  if (s.size() < substr.size()){
    return false;
  }
  return s.substr(s.size() - substr.size()) == substr;
}

}
