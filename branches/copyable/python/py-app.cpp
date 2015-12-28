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

#include "app/get-app-context.hh"
#include "python/py-include.hh"
#include "python/py-app.hh"
#include "python/py-ugly-forward.hh"
#include "text/formatting.hh"
#include "util/file-path.hh"
#include "util/grid.hh"

namespace faint{

AppContext& get_cpp_object(faintAppObject*){
  return get_app_context();
}

bool faint_side_ok(faintAppObject*){
  return true;
}

void show_error(faintAppObject*){
}

static void faintapp_set_transparency_indicator(AppContext& app, const ColRGB& color){
  app.SetTransparencyStyle(TransparencyStyle(color));
}

static void faintapp_set_checkered_transparency_indicator(AppContext& app){
  app.SetTransparencyStyle(TransparencyStyle());
}

static bool faintapp_get_checkered_transparency_indicator(AppContext& app){
  return app.GetTransparencyStyle().IsCheckered();
}

static void faintapp_open_files(AppContext& app, const std::vector<utf8_string>& pathStrings){
  FileList paths;
  for (const utf8_string& pathStr : pathStrings){
    if (!is_file_path(pathStr)){
      throw ValueError(space_sep(quoted(pathStr), "is not a file path"));
    }
    if (!is_absolute_path(pathStr)){
      throw ValueError(space_sep("Path", quoted(pathStr), "not absolute."));
    }

    paths.push_back(FilePath::FromAbsolute(pathStr));
  }
  if (paths.empty()){
    return;
  }

  app.Load(paths);

}


// Method table for the Python interface class for the Faint app
static PyMethodDef faintapp_methods[] = {
  {"set_transparency_indicator",
   FORWARDER(faintapp_set_transparency_indicator), METH_VARARGS,
   "set_transparency_indicator(r,g,b)\nSets a color for indicating alpha transparency (instead of the checkered pattern)."},

  {"open",
   FORWARDER(faintapp_open_files), METH_VARARGS,
   "open([filepath1, filepath2,...])\nOpen the specified image files in new tabs."},

  {"set_checkered_transparency_indicator",
   FORWARDER(faintapp_set_checkered_transparency_indicator), METH_NOARGS,
   "set_checkered_transparency_indicator()\nUse a checkered pattern to indicate alpha transparency."},

  {"get_checkered_transparency_indicator",
   FORWARDER(faintapp_get_checkered_transparency_indicator),
   METH_NOARGS,
   "get_checkered_transparency_indicator()\nTrue if a checkered pattern is used to indicate transparency."},
  {0, 0, 0, 0}  // Sentinel
};

Color faintapp_getter_grid_color(AppContext& app){
  return app.GetDefaultGrid().GetColor();
}

bool faintapp_getter_grid_dashed(AppContext& app){
  return app.GetDefaultGrid().Dashed();
}

static int faintapp_init(faintAppObject*, PyObject*, PyObject*) {
  return init_ok;
}

static PyObject* faintapp_new(PyTypeObject* type, PyObject*, PyObject*){
  faintAppObject* self = (faintAppObject*)type->tp_alloc(type, 0);
  return (PyObject *)self;
}

static PyObject* faintapp_repr(faintAppObject*){
  return Py_BuildValue("s", "FaintApp");
}

void faintapp_setter_grid_color(AppContext& app, const Color& gridColor){
  Grid grid(app.GetDefaultGrid());
  grid.SetColor(gridColor);
  app.SetDefaultGrid(grid);
}

void faintapp_setter_grid_dashed(AppContext& app, const bool& dashed){
  Grid grid(app.GetDefaultGrid());
  grid.SetDashed(dashed);
  app.SetDefaultGrid(grid);
}

static PyGetSetDef faintapp_getseters[] = {
  {(char*)"gridcolor", // Fixme: Use a grid-object instead.
   GET_FORWARDER(faintapp_getter_grid_color),
   SET_FORWARDER(faintapp_setter_grid_color),
   (char*)"Color of grids", nullptr},

  {(char*)"griddashed",
   GET_FORWARDER(faintapp_getter_grid_dashed),
   SET_FORWARDER(faintapp_setter_grid_dashed),
   (char*)"Whether grids are dashed", nullptr},
  {0,0,0,0,0} // Sentinel
};

PyTypeObject FaintAppType = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "FaintApp", //tp_name
    sizeof(faintAppObject), // tp_basicsize
    0, // tp_itemsize
    0, // tp_dealloc
    0, // tp_print
    0, // tp_getattr
    0, // tp_setattr
    0, // tp_compare
    (reprfunc)faintapp_repr, // tp_repr
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
    "The Faint Application Window", // tp_doc
    0, // tp_traverse
    0, // tp_clear
    0, // tp_richcompare
    0, // tp_weaklistoffset
    0, // tp_iter
    0, // tp_iternext
    faintapp_methods, // tp_methods
    0, // tp_members
    faintapp_getseters, // tp_getset
    0, // tp_base
    0, // tp_dict
    0, // tp_descr_get
    0, // tp_descr_set
    0, // tp_dictoffset
    (initproc)faintapp_init, // tp_init
    0, // tp_alloc
    faintapp_new, // tp_new
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
