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

#ifndef FAINT_PY_CREATE_OBJECT_HH
#define FAINT_PY_CREATE_OBJECT_HH
#include "objects/object.hh"
#include "python/py-include.hh"

namespace faint{

// Functions for creating C++-Faint-Objects from Python-argument lists.
// (Fixme: Make Settings attribute optional, to allow convenient
// creation from interpreter)
Object* ellipse_from_py_args(PyObject*);
Object* line_from_py_args(PyObject*);
Object* path_from_py_args(PyObject*);
Object* polygon_from_py_args(PyObject*);
Object* raster_from_py_args(PyObject*);
Object* rectangle_from_py_args(PyObject*);
Object* spline_from_py_args(PyObject*);
Object* text_from_py_args(PyObject*);

} // namespace

#endif
