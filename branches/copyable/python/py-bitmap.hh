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

#ifndef FAINT_PY_BITMAP_HH
#define FAINT_PY_BITMAP_HH
#include "python/mapped-type.hh"
#include "util/common-fwd.hh"

namespace faint{

extern PyTypeObject BitmapType;
typedef struct {
  PyObject_HEAD
  Bitmap* bmp;
} bitmapObject;


template<>
struct MappedType<Bitmap&>{
  typedef bitmapObject PYTHON_TYPE;
};

inline Bitmap& get_cpp_object(bitmapObject* self){
  return *self->bmp;
}

bool faint_side_ok(bitmapObject*);
void show_error(bitmapObject*);

} // namespace

#endif
