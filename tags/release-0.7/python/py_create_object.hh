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

#include "pythoninclude.hh"
#include "objects/object.hh"

// Utility functions for creating C++-Faint-Object from
// Python-argument lists to interface functions
// (Fixme: Make Settings attribute optional, to allow convenient
// creation from interpreter)
Object* CreateEllipse(PyObject*);
Object* CreateLine(PyObject*);
Object* CreatePath(PyObject*);
Object* CreatePolygon(PyObject*);
Object* CreateRaster(PyObject*);
Object* CreateRectangle(PyObject*);
Object* CreateSpline(PyObject*);
Object* CreateText(PyObject*);
