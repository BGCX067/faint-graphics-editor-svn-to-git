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
#include "python/py_app.hh"
#include "python/py_app_objects.hh"
#include "python/py_canvas.hh"
#include "python/py_grid.hh"
#include "python/py_imageprops.hh"
#include "python/py_settings.hh"
#include "python/py_something.hh"
#include "python/py_tri.hh"
#include "python/py_util.hh"

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
  return 0;
}

static PyObject* faintwindow_repr(faintWindowObject*){
  return Py_BuildValue("s", "FaintWindow");
}

PyTypeObject FaintWindowType = {
    PyObject_HEAD_INIT(NULL)
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
    return 0;
  }
  GetAppContext().SetInterpreterBackground( c );
  return Py_BuildValue("");
}

static PyObject* faintinterpreter_set_text_color(faintInterpreterObject*, PyObject* args ){
  faint::Color c;
  if ( !parse_rgb_color(args, c ) ){
    return 0;
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
  return 0;
}

static PyObject* faintinterpreter_new(PyTypeObject* type, PyObject*, PyObject* ){
  faintInterpreterObject* self = (faintInterpreterObject*)type->tp_alloc(type, 0);
  return (PyObject *)self;
}

static PyObject* faintinterpreter_repr(faintWindowObject*){
  return Py_BuildValue("s", "FaintInterpreter");
}

PyTypeObject FaintInterpreterType = {
    PyObject_HEAD_INIT(NULL)
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
    return 0;
  }
  GetAppContext().AddPaletteColor(c);
  return Py_BuildValue("");
}

static PyObject* faintpalette_set(faintInterpreterObject*, PyObject* args ){
  PyObject* sequence = 0;
  if ( !PyArg_ParseTuple(args, "O", &sequence) ){
    return 0;
  }
  if ( !PySequence_Check(sequence) ){
    PyErr_SetString(PyExc_TypeError, "Not a sequence"); // Fixme
    return 0;
  }

  const int n = PySequence_Length(sequence);
  if ( n == 0 ){
    PyErr_SetString(PyExc_ValueError, "Empty sequence"); // Fixme
    return 0;
  }

  std::vector<faint::Color> colors;
  for ( int i = 0; i != n; i++ ){
    PyObject* colorTuple = PySequence_GetItem(sequence, i);
    faint::Color c;
    if ( !parse_color(colorTuple, c) ){
      PyErr_SetString(PyExc_ValueError, "Atleast 1 non-color"); // Fixme
      return 0;
    }
    colors.push_back(c);
  }
  GetAppContext().SetPalette(colors);
  return Py_BuildValue("");
}

static PyMethodDef faintpalette_methods[] = {
  {"add", (PyCFunction)faintpalette_add, METH_VARARGS, "Adds a color to the palette." },
  {"set", (PyCFunction)faintpalette_set, METH_VARARGS, "Sets the palette to the specified list of colors."},
  {0, 0, 0, 0}  // Sentinel
};

static int faintpalette_init(faintWindowObject*, PyObject*, PyObject*) {
  return 0;
}

static PyObject* faintpalette_new(PyTypeObject* type, PyObject*, PyObject*){
  faintInterpreterObject* self = (faintInterpreterObject*)type->tp_alloc(type, 0);
  return (PyObject *)self;
}

static PyObject* faintpalette_repr(faintWindowObject* ){
  return Py_BuildValue("s", "Faint Palette");
}

PyTypeObject FaintPaletteType = {
    PyObject_HEAD_INIT(NULL)
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
  SmthType.tp_new = PyType_GenericNew;
  int result = PyType_Ready(&SmthType);
  assert( result >= 0 );
  Py_INCREF( &SmthType );
  PyModule_AddObject( module, "Something", (PyObject* )&SmthType );

  CanvasType.tp_new = PyType_GenericNew;
  result = PyType_Ready(&CanvasType);
  assert( result >= 0 );
  Py_INCREF( &CanvasType );
  PyModule_AddObject( module, "Canvas", (PyObject* )&CanvasType );

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

  GridType.tp_new = PyType_GenericNew;
  result = PyType_Ready(&GridType);
  assert( result >= 0 );
  Py_INCREF( &GridType );
  PyModule_AddObject( module, "Grid", (PyObject* )&GridType );
}
