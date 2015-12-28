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

#include "app/getappcontext.hh"
#include "python/pythoninclude.hh"
#include "python/py-app.hh"
#include "python/py-app-objects.hh"
#include "python/py-bitmap.hh"
#include "python/py-canvas.hh"
#include "python/py-frame.hh"
#include "python/py-linear-gradient.hh"
#include "python/py-radial-gradient.hh"
#include "python/py-frameprops.hh"
#include "python/py-grid.hh"
#include "python/py-imageprops.hh"
#include "python/py-pattern.hh"
#include "python/py-settings.hh"
#include "python/py-something.hh"
#include "python/py-tri.hh"
#include "python/py-util.hh"
#include "util/draw-source-map.hh"

// faintwindow
static PyObject* faintwindow_maximize(faintWindowObject*, PyObject* ){
  GetAppContext().Maximize();
  return Py_BuildValue("");
}

// Method table for the Python interface class for the Faint main frame
static PyMethodDef faintwindow_methods[] = {
  {"maximize", (PyCFunction)faintwindow_maximize, METH_NOARGS, "Maximize or de-maximize the window" },
  {0, 0, 0, 0}  // Sentinel
};

static PyObject* faintwindow_new(PyTypeObject* type, PyObject*, PyObject*){
  faintWindowObject* self = ( faintWindowObject*)type->tp_alloc(type, 0);
  return (PyObject *)self;
}

static int faintwindow_init(faintWindowObject*, PyObject*, PyObject*) {
  return faint::init_ok;
}

static PyObject* faintwindow_repr(faintWindowObject*){
  return Py_BuildValue("s", "FaintWindow");
}

PyTypeObject FaintWindowType = {
    PyObject_HEAD_INIT(nullptr)
    0, // ob_size
    "FaintWindow", //tp_name
    sizeof(faintWindowObject), // tp_basicsize
    0, // tp_itemsize
    0, // tp_dealloc
    0, // tp_print
    0, // tp_getattr
    0, // tp_setattr
    0, // tp_compare
    (reprfunc)faintwindow_repr, // tp_repr
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
    faintwindow_methods, // tp_methods
    0, // tp_members
    0, // tp_getset
    0, // tp_base
    0, // tp_dict
    0, // tp_descr_get
    0, // tp_descr_set
    0, // tp_dictoffset
    (initproc)faintwindow_init, // tp_init
    0, // tp_alloc
    faintwindow_new, // tp_new
    0, // tp_free
    0, // tp_is_gc
    0, // tp_bases
    0, // tp_mro
    0, // tp_cache
    0, // tp_subclasses
    0, // tp_weaklist
    0, // tp_del
    0 // tp_version_tag
};

// faintinterpreter
static PyObject* faintinterpreter_maximize( faintInterpreterObject*, PyObject* ){
  GetAppContext().MaximizeInterpreter();
  return Py_BuildValue("");
}

static PyObject* faintinterpreter_set_background(faintInterpreterObject*, PyObject* args ){
  faint::Color c;
  if ( !parse_rgb_color( args, c ) ){
    return nullptr;
  }
  GetAppContext().SetInterpreterBackground( c );
  return Py_BuildValue("");
}

static PyObject* faintinterpreter_set_text_color(faintInterpreterObject*, PyObject* args ){
  faint::Color c;
  if ( !parse_rgb_color(args, c ) ){
    return nullptr;
  }
  GetAppContext().SetInterpreterTextColor( c );
  return Py_BuildValue("");
}

static PyMethodDef faintinterpreter_methods[] = {
  {"maximize", (PyCFunction)faintinterpreter_maximize, METH_NOARGS, "Maximize the Python interpreter window."},
  {"set_background_color", (PyCFunction)faintinterpreter_set_background, METH_VARARGS, "Set the Python interpreter background color." },
  {"set_text_color", (PyCFunction)faintinterpreter_set_text_color, METH_VARARGS, "Set the Python interpreter text color." },
  {0,0,0,0} // Sentinel
};

static int faintinterpreter_init(faintWindowObject*, PyObject*, PyObject*) {
  return faint::init_ok;
}

static PyObject* faintinterpreter_new(PyTypeObject* type, PyObject*, PyObject* ){
  faintInterpreterObject* self = (faintInterpreterObject*)type->tp_alloc(type, 0);
  return (PyObject *)self;
}

static PyObject* faintinterpreter_repr(faintWindowObject*){
  return Py_BuildValue("s", "FaintInterpreter");
}

PyTypeObject FaintInterpreterType = {
    PyObject_HEAD_INIT(nullptr)
    0, // ob_size
    "FaintInterpreter", // tp_name
    sizeof(faintInterpreterObject), // tp_basicsize
    0, // tp_itemsize
    0, // tp_dealloc
    0, // tp_print
    0, // tp_getattr
    0, // tp_setattr
    0, // tp_compare
    (reprfunc)faintinterpreter_repr, // tp_repr
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
    "Faint Interpreter Interface", // tp_doc
    0, // tp_traverse
    0, // tp_clear
    0, // tp_richcompare
    0, // tp_weaklistoffset
    0, // tp_iter
    0, // tp_iternext
    faintinterpreter_methods, // tp_methods
    0, // tp_members
    0, // tp_getset
    0, // tp_base
    0, // tp_dict
    0, // tp_descr_get
    0, // tp_descr_set
    0, // tp_dictoffset
    (initproc)faintinterpreter_init, // tp_init
    0, // tp_alloc
    faintinterpreter_new, // tp_new
    0, // tp_free
    0, // tp_is_gc
    0, // tp_bases
    0, // tp_mro
    0, // tp_cache
    0, // tp_subclasses
    0, // tp_weaklist
    0, // tp_del
    0, // tp_version_tag
};

// Palette
static PyObject* faintpalette_add(faintInterpreterObject*, PyObject* args ){
  faint::Color c;
  if ( !parse_color( args, c ) ){
    return nullptr;
  }
  GetAppContext().AddPaletteColor(c);
  return Py_BuildValue("");
}

static PyObject* faintpalette_set(faintInterpreterObject*, PyObject* args ){
  PyObject* sequence = nullptr;
  if ( !PyArg_ParseTuple(args, "O", &sequence) ){
    return nullptr;
  }
  if ( !PySequence_Check(sequence) ){
    PyErr_SetString(PyExc_TypeError, "Sequence required");
    return nullptr;
  }

  const int n = PySequence_Length(sequence);
  if ( n == 0 ){
    PyErr_SetString(PyExc_ValueError, "At least one color required");
    return nullptr;
  }

  std::vector<faint::DrawSource> sources;
  for ( int i = 0; i != n; i++ ){
    faint::ScopedRef pyObj(PySequence_GetItem(sequence, i));
    faint::DrawSource src;
    if ( !parse_draw_source(*pyObj, src) ){
      PyErr_SetString(PyExc_ValueError, "At least one Invalid color in sequence");
      return nullptr;
    }
    sources.push_back(src);
  }
  faint::DrawSourceMap drawSourceMap;
  for ( size_t i = 0; i != sources.size(); i++ ){
    drawSourceMap.Append(sources[i]);
  }
  GetAppContext().SetPalette(drawSourceMap);
  return Py_BuildValue("");
}

static PyMethodDef faintpalette_methods[] = {
  {"add", (PyCFunction)faintpalette_add, METH_VARARGS, "Adds a color to the palette." },
  {"set", (PyCFunction)faintpalette_set, METH_VARARGS, "Sets the palette to the specified list of colors."},
  {0, 0, 0, 0}  // Sentinel
};

static int faintpalette_init(faintWindowObject*, PyObject*, PyObject*) {
  return faint::init_ok;
}

static PyObject* faintpalette_new(PyTypeObject* type, PyObject*, PyObject*){
  faintInterpreterObject* self = (faintInterpreterObject*)type->tp_alloc(type, 0);
  return (PyObject *)self;
}

static PyObject* faintpalette_repr(faintWindowObject* ){
  return Py_BuildValue("s", "Faint Palette");
}

PyTypeObject FaintPaletteType = {
    PyObject_HEAD_INIT(nullptr)
    0, // ob_size
    "FaintPalette", // tp_name
    sizeof(faintPaletteObject), // tp_basicsize
    0, // tp_itemsize
    0, // tp_dealloc
    0, // tp_print
    0, // tp_getattr
    0, // tp_setattr
    0, // tp_compare
    (reprfunc)faintpalette_repr, // tp_repr
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
    "Faint Palette interface", // tp_doc
    0, // tp_traverse
    0, // tp_clear
    0, // tp_richcompare
    0, // tp_weaklistoffset
    0, // tp_iter
    0, // tp_iternext
    faintpalette_methods, // tp_methods
    0, // tp_members
    0, // tp_getset
    0, // tp_base
    0, // tp_dict
    0, // tp_descr_get
    0, // tp_descr_set
    0, // tp_dictoffset
    (initproc)faintpalette_init, // tp_init
    0, // tp_alloc
    faintpalette_new, // tp_new
    0, // tp_free
    0, // tp_is_gc
    0, // tp_bases
    0, // tp_mro
    0, // tp_cache
    0, // tp_subclasses
    0, // tp_weaklist
    0, // tp_del
    0, // tp_version_tag
};

void AddPyObjects( PyObject* module ){
  BitmapType.tp_new = PyType_GenericNew;
  int result = PyType_Ready(&BitmapType);
  assert( result >= 0 );
  Py_INCREF( &BitmapType );
  PyModule_AddObject( module, "Bitmap", (PyObject* )&BitmapType );

  PatternType.tp_new = PyType_GenericNew;
  result = PyType_Ready(&PatternType);
  assert( result >= 0 );
  Py_INCREF( &PatternType );
  PyModule_AddObject( module, "Pattern", (PyObject* )&PatternType );

  SmthType.tp_new = PyType_GenericNew;
  result = PyType_Ready(&SmthType);
  assert( result >= 0 );
  Py_INCREF( &SmthType );
  PyModule_AddObject( module, "Something", (PyObject* )&SmthType );

  CanvasType.tp_new = PyType_GenericNew;
  result = PyType_Ready(&CanvasType);
  assert( result >= 0 );
  Py_INCREF( &CanvasType );
  PyModule_AddObject( module, "Canvas", (PyObject* )&CanvasType );

  FrameType.tp_new = PyType_GenericNew;
  result = PyType_Ready(&FrameType);
  assert( result >= 0 );
  Py_INCREF( &FrameType );
  PyModule_AddObject( module, "Frame", (PyObject* )&FrameType );

  LinearGradientType.tp_new = PyType_GenericNew;
  result = PyType_Ready(&LinearGradientType);
  assert( result >= 0 );
  Py_INCREF( &LinearGradientType );
  PyModule_AddObject( module, "LinearGradient", (PyObject*)&LinearGradientType );

  RadialGradientType.tp_new = PyType_GenericNew;
  result = PyType_Ready(&RadialGradientType);
  assert( result >= 0 );
  Py_INCREF( &RadialGradientType );
  PyModule_AddObject( module, "RadialGradient", (PyObject*)&RadialGradientType );

  SettingsType.tp_new = PyType_GenericNew;
  result = PyType_Ready(&SettingsType);
  assert( result >= 0 );
  Py_INCREF( &SettingsType );
  PyModule_AddObject( module, "Settings", (PyObject*)&SettingsType );

  FaintAppType.tp_new = PyType_GenericNew;
  result = PyType_Ready(&FaintAppType);
  assert( result >= 0 );
  Py_INCREF( &FaintAppType );
  PyModule_AddObject( module, "FaintApp", (PyObject*)&FaintAppType );

  FaintWindowType.tp_new = PyType_GenericNew;
  result = PyType_Ready(&FaintWindowType);
  assert( result >= 0 );
  Py_INCREF( &FaintWindowType );
  PyModule_AddObject( module, "FaintWindow", (PyObject*)&FaintWindowType );

  FaintInterpreterType.tp_new = PyType_GenericNew;
  result = PyType_Ready(&FaintInterpreterType);
  assert( result >= 0 );
  Py_INCREF( &FaintInterpreterType );
  PyModule_AddObject( module, "FaintInterpreter", (PyObject*)&FaintInterpreterType );

  FaintPaletteType.tp_new = PyType_GenericNew;
  result = PyType_Ready(&FaintPaletteType);
  assert( result >= 0 );
  Py_INCREF( &FaintPaletteType );
  PyModule_AddObject( module, "FaintPalette", (PyObject*)&FaintPaletteType );

  TriType.tp_new = PyType_GenericNew;
  result = PyType_Ready(&TriType);
  assert( result >= 0 );
  Py_INCREF( &TriType );
  PyModule_AddObject( module, "Tri", (PyObject* )&TriType );

  ImagePropsType.tp_new = PyType_GenericNew;
  result = PyType_Ready(&ImagePropsType);
  assert( result >= 0 );
  Py_INCREF( &ImagePropsType );
  PyModule_AddObject( module, "ImageProps", (PyObject* )&ImagePropsType );

  FramePropsType.tp_new = PyType_GenericNew;
  result = PyType_Ready(&FramePropsType);
  assert( result >= 0 );
  Py_INCREF( &FramePropsType );
  PyModule_AddObject( module, "FrameProps", (PyObject* )&FramePropsType );

  GridType.tp_new = PyType_GenericNew;
  result = PyType_Ready(&GridType);
  assert( result >= 0 );
  Py_INCREF( &GridType );
  PyModule_AddObject( module, "Grid", (PyObject* )&GridType );

  PyObject* binds = PyDict_New();
  PyModule_AddObject( module, "_binds", binds );
  PyModule_AddObject( module, "LoadError", faint::get_load_exception_type() );
  PyModule_AddObject( module, "SaveError", faint::get_save_exception_type() );
}
