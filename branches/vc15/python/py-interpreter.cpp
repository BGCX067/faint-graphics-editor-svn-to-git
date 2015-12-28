// -*- coding: us-ascii-unix -*-
// Copyright 2014 Lukas Kemmer
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
#include "python/mapped-type.hh"
#include "python/py-include.hh"
#include "python/py-interpreter.hh"
#include "python/py-ugly-forward.hh"

namespace faint{
template<>
struct MappedType<AppContext&>{
  using PYTHON_TYPE = faintInterpreterObject;
  static AppContext& GetCppObject(faintInterpreterObject*){
    return get_app_context();
  }

  static bool Expired(faintInterpreterObject*){
    // Always ok
    return false;
  }

  static void ShowError(faintInterpreterObject*){
  // Never broken
  }

  static utf8_string DefaultRepr(const faintInterpreterObject*){
    return "FaintInterpreter";
  }
};

void add_interpreter_to_module(PyObject* module){
  FaintInterpreterType.tp_new = PyType_GenericNew;
  int result = PyType_Ready(&FaintInterpreterType);
  assert(result >= 0);
  Py_INCREF(&FaintInterpreterType);
  PyModule_AddObject(module, "FaintInterpreter", (PyObject*)&FaintInterpreterType);
}

/* method: "maximize()\n
Maximize or de-maximize the interpreter window." */
static void faintinterpreter_maximize(AppContext& app){
  app.MaximizeInterpreter();
}

/* method: "set_background_color(r,g,b)\n
Set the background color of the interpreter window" */
static void faintinterpreter_set_background_color(AppContext& app, const ColRGB& c){
  app.SetInterpreterBackground(c);
}

/* method: "set_text_color(r,g,b)\n
Sets the text color of the interpreter window." */
static void faintinterpreter_set_text_color(AppContext& app, const ColRGB& c){
  app.SetInterpreterTextColor(c);
}

static PyMethodDef faintinterpreter_methods[] = {
  FORWARDER(faintinterpreter_maximize, METH_NOARGS, "maximize", "Maximize the Python interpreter window."),
  FORWARDER(faintinterpreter_set_background_color, METH_VARARGS, "set_background_color", "Set the Python interpreter background color."),
  FORWARDER(faintinterpreter_set_text_color, METH_VARARGS, "set_text_color", "Set the Python interpreter text color."),
  {0,0,0,0} // Sentinel
};

static void faintinterpreter_init(faintInterpreterObject&){
}

static PyObject* faintinterpreter_new(PyTypeObject* type, PyObject*, PyObject*){
  faintInterpreterObject* self = (faintInterpreterObject*)type->tp_alloc(type, 0);
  return (PyObject *)self;
}

static utf8_string faintinterpreter_repr(AppContext&){
  return "FaintInterpreter";
}

PyTypeObject FaintInterpreterType = {
  PyVarObject_HEAD_INIT(nullptr, 0)
  "FaintInterpreter", // tp_name
  sizeof(faintInterpreterObject), // tp_basicsize
  0, // tp_itemsize
  0, // tp_dealloc
  0, // tp_print
  0, // tp_getattr
  0, // tp_setattr
  0, // tp_compare
  REPR_FORWARDER(faintinterpreter_repr), // tp_repr
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
  INIT_FORWARDER(faintinterpreter_init), // tp_init
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
  0  // tp_finalize
};


} // namespace
