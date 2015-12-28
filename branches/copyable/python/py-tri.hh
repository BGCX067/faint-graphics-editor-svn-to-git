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

#ifndef FAINT_PY_TRI_HH
#define FAINT_PY_TRI_HH
#include "geo/tri.hh"
#include "python/mapped-type.hh"

namespace faint{

extern PyTypeObject TriType;
typedef struct {
  PyObject_HEAD
  Tri tri;
} triObject;

template<>
struct MappedType<Tri&>{
  typedef triObject PYTHON_TYPE;
};

Tri& get_cpp_object(triObject* self);
bool faint_side_ok(triObject*);
void show_error(triObject*);

PyObject* pythoned(const Tri&);

} // namespace

#endif
