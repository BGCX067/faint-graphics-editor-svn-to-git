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

#ifndef FAINT_PY_RADIAL_GRADIENT_HH
#define FAINT_PY_RADIAL_GRADIENT_HH
#include "python/mapped-type.hh"
#include "util/gradient.hh"

namespace faint{

extern PyTypeObject RadialGradientType;
typedef struct{
  PyObject_HEAD
  RadialGradient* gradient;
} radialGradientObject;

template<>
struct MappedType<RadialGradient&>{
  typedef radialGradientObject PYTHON_TYPE;
};

RadialGradient& get_cpp_object(radialGradientObject*);
bool faint_side_ok(radialGradientObject*);
void show_error(radialGradientObject*);

} // namespace

#endif
