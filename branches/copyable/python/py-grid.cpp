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

#include <sstream>
#include "app/get-app-context.hh"
#include "python/py-include.hh"
#include "python/py-grid.hh"
#include "python/py-ugly-forward.hh"
#include "python/py-interface.hh"
#include "util/canvas.hh"
#include "util/grid.hh"
#include "util/optional.hh"

namespace faint{

static bool grid_ok_no_error(gridObject* self){
  if ( self->canvas == nullptr ){
    // A null-canvas means the active canvas is the target, There
    // should always be atleast one canvas.
    return true;
  }
  else if ( get_app_context().Exists(self->canvasId) ){
    return true;
  }
  return false;
}

CanvasGrid get_cpp_object(gridObject* grid){
  if ( grid->canvas == nullptr ){
    return CanvasGrid(&get_app_context().GetActiveCanvas());
  }
  return CanvasGrid(grid->canvas);
}

bool faint_side_ok(gridObject* grid){
  return grid_ok_no_error(grid);
}

void show_error(gridObject*){
  PyErr_SetString(PyExc_ValueError, "Canvas for grid removed.");
}

// Returns true if the canvas for the grid exists.
// Otherwise, sets a python error and returns false.
bool grid_ok( gridObject* self ){
  if ( grid_ok_no_error(self) ){
    return true;
  }
  PyErr_SetString(PyExc_ValueError, "Canvas for grid removed.");
  return false;
}

static Canvas& get_canvas( gridObject* self ){
  return self->canvas == nullptr ?
    get_app_context().GetActiveCanvas() :
    *self->canvas;
}

Optional<Grid> get_grid( gridObject* self ){
  if ( !grid_ok(self) ){
    return no_option();
  }
  return option(get_canvas(self).GetGrid());
}

static PyObject* grid_new( PyTypeObject* type, PyObject*, PyObject* ){
  gridObject* self = (gridObject*)type->tp_alloc(type, 0);
  return (PyObject*)self;
}

static int grid_init(gridObject* self, PyObject* /*args*/, PyObject*){
  self->canvas = nullptr;
  self->canvasId = CanvasId::DefaultID();
  return faint::init_ok;
}

PyObject* py_grid_canvas( Canvas* canvas ){
  gridObject* py_grid = (gridObject*) GridType.tp_alloc(&GridType,0);
  py_grid->canvas = canvas;
  if ( canvas != nullptr ){
    py_grid->canvasId = canvas->GetId();
  }
  return (PyObject*) py_grid;
}

Point grid_getter_anchor( const CanvasGrid& self ){
  return self.canvas->GetGrid().Anchor();
}

void grid_setter_anchor( const CanvasGrid& self, const Point& anchor ){
  Grid g = self.canvas->GetGrid();
  g.SetAnchor(anchor);
  self.canvas->SetGrid(g);
  faint::python_queue_refresh(*self.canvas);
}

faint::Color grid_getter_color( const CanvasGrid& self ){
  return self.canvas->GetGrid().GetColor();
}

void grid_setter_color( const CanvasGrid& self, const faint::Color& c ){
  Grid g = self.canvas->GetGrid();
  g.SetColor(c);
  self.canvas->SetGrid(g);
  faint::python_queue_refresh(*self.canvas);
}

bool grid_getter_dashed( const CanvasGrid& self ){
  return self.canvas->GetGrid().Dashed();
}

void grid_setter_dashed( const CanvasGrid& self, const bool& dashed ){
  Canvas& canvas = *self.canvas;
  Grid g = canvas.GetGrid();
  g.SetDashed(dashed);
  canvas.SetGrid(g);
  faint::python_queue_refresh(canvas);
}

int grid_getter_spacing( const CanvasGrid& self ){
  return self.canvas->GetGrid().Spacing();
}

void grid_setter_spacing( const CanvasGrid& self, const int& spacing ){
  if ( spacing <= 0 ){
    throw faint::ValueError("Grid spacing must be > 0");
  }
  Canvas& canvas = *self.canvas;
  Grid g = canvas.GetGrid();
  g.SetSpacing(spacing);
  canvas.SetGrid(g);
  faint::python_queue_refresh(canvas);
}

bool grid_getter_visible( const CanvasGrid& self ){
  return self.canvas->GetGrid().Visible();
}

void grid_setter_visible( const CanvasGrid& self, const bool& visible ){
  Canvas& canvas = *self.canvas;
  Grid g = canvas.GetGrid();
  g.SetVisible(visible);
  canvas.SetGrid(g);
  faint::python_queue_refresh(canvas);
}

static PyObject* grid_repr(gridObject* self ){
  if ( self->canvas == nullptr ){
    return Py_BuildValue("s", "Grid (active canvas)");
  }

  std::stringstream ss;
  if ( grid_ok_no_error(self) ){
    ss << "Grid for Canvas #" << self->canvasId.Raw();
  }
  else{
    ss << "Grid for removed Canvas #" << self->canvasId.Raw();
  }
  return Py_BuildValue("s", ss.str().c_str());
}

static PyGetSetDef grid_getseters[] = {
  {(char*)"anchor",
   GET_FORWARDER(grid_getter_anchor),
   SET_FORWARDER(grid_setter_anchor),
   (char*)"Grid anchor (x,y-tuple)\nSpecifies a point that will be intersected by the grid.", nullptr},

  {(char*)"color",
   GET_FORWARDER(grid_getter_color),
   SET_FORWARDER(grid_setter_color),
   (char*)"Grid color (rgba-tuple)", nullptr},

  {(char*)"dashed",
   GET_FORWARDER(grid_getter_dashed),
   SET_FORWARDER(grid_setter_dashed),
   (char*)"Grid dashes (boolean)\nSpecifies whether the grid should be dashed or solid.", nullptr},

  {(char*)"spacing",
   GET_FORWARDER(grid_getter_spacing),
   SET_FORWARDER(grid_setter_spacing),
   (char*)"Spacing (px)\nSpecifies the spacing between grid lines in pixels.", nullptr},

  {(char*)"visible",
   GET_FORWARDER(grid_getter_visible),
   SET_FORWARDER(grid_setter_visible),
   (char*)"Visibility (True/False)\nSpecifies whether the grid should be visible.", nullptr},
  {0,0,0,0,0} // Sentinel
};

PyTypeObject GridType = {
  PyVarObject_HEAD_INIT(nullptr, 0)
  "Grid", // tp_name
  sizeof(gridObject), //tp_basicsize
  0, // tp_itemsize
  0, // tp_dealloc
  0, // tp_print
  0, // tp_getattr
  0, // tp_setattr
  0, // tp_compare
  (reprfunc)grid_repr, // tp_repr
  0, // tp_as_number
  0, // tp_as_sequence
  0, // tp_as_mapping
  0, // tp_hash
  0, // tp_call
  0, // tp_str
  0, // tp_getattro
  0, // tp_setattro
  0, // tp_as_buffer
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
  "Interface for adjusting the guiding/snapping grids in an image.", // tp_doc
  0, // tp_traverse */
  0, // tp_clear */
  0, // tp_richcompare */
  0, // tp_weaklistoffset */
  0, // tp_iter */
  0, // tp_iternext */
  0, // tp_methods */
  0, // tp_members
  grid_getseters, // tp_getset */
  0, // tp_base */
  0, // tp_dict */
  0, // tp_descr_get */
  0, // tp_descr_set */
  0, // tp_dictoffset */
  (initproc)grid_init, // tp_init
  0, // tp_alloc
  grid_new, // tp_new */
  0, // tp_free
  0, // tp_is_gc
  0, // tp_bases
  0, // tp_mro
  0, // tp_cache
  0, // tp_subclasses
  0, // tp_weaklist
  0, // tp_del
  0, // tp_version_tag
  0  // tp_finalize
};

} // namespace
