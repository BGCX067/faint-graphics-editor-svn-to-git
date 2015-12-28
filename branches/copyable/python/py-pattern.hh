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

#ifndef FAINT_PY_PATTERN_HH
#define FAINT_PY_PATTERN_HH
#include "python/mapped-type.hh"
#include "util/pattern.hh"

namespace faint{

extern PyTypeObject PatternType;
typedef struct{
  PyObject_HEAD
  Pattern* pattern;
} patternObject;

template<>
struct MappedType<Pattern&>{
  typedef patternObject PYTHON_TYPE;
};

Pattern& get_cpp_object(patternObject*);
bool faint_side_ok(patternObject*);
void show_error(patternObject*);

} // namespace

#endif
