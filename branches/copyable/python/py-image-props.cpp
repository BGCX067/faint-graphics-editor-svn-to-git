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

#include "python/py-include.hh"
#include "python/py-frame-props.hh"
#include "python/py-image-props.hh"
#include "python/py-ugly-forward.hh"
#include "util/image-props.hh"

namespace faint{

bool faint_side_ok(imagePropsObject*){
  // Fixme: Need a way to determine if the contained object is still alive
  return true;
}

void show_error(imagePropsObject*){
  PyErr_SetString(PyExc_ValueError, "Operation attempted on bad ImageProps.");
}

static PyObject* get_frameprops(imagePropsObject* self, int index){
  framePropsObject* py_frameProps = (framePropsObject*)(FramePropsType.tp_alloc(&FramePropsType,0));
  py_frameProps->imageProps = self->props;
  py_frameProps->frame_index = index;
  return (PyObject*)py_frameProps;
}

static PyObject* get_frameprops(ImageProps& self, int index){
  framePropsObject* py_frameProps = (framePropsObject*)(FramePropsType.tp_alloc(&FramePropsType,0));
  py_frameProps->imageProps = &self;
  py_frameProps->frame_index = index;
  return (PyObject*)py_frameProps;
}

static PyObject* imageprops_get_frame(imagePropsObject* self, PyObject* args){
  int frameId;
  if ( !PyArg_ParseTuple( args, "i", &frameId) ){
    return nullptr;
  }
  if ( frameId < 0 ){
    PyErr_SetString(PyExc_ValueError, "Negative index specified");
    return nullptr;
  }
  if ( self->props->GetNumFrames() <= frameId ){
    PyErr_SetString(PyExc_ValueError, "Invalid frame index specified");
    return nullptr;
  }

  return get_frameprops(self, frameId);
}

void imageprops_set_error( ImageProps& self, const faint::utf8_string& err ){
  self.SetError(err);
}

void imageprops_add_warning( ImageProps& self, const faint::utf8_string& warn ){
  self.AddWarning(warn);
}

PyObject* imageprops_add_frame( ImageProps& self, const Optional<IntSize>& size){
  self.AddFrame(ImageInfo(size.Or(IntSize(640,480)), create_bitmap(false)));
  return get_frameprops(self, self.GetNumFrames() - 1);
}

// Method table for Python-ImageProperties interface (excluding
// standard functions like del, repr and str)
static PyMethodDef image_props_methods[] = {
  {"add_warning",FORWARDER(imageprops_add_warning), METH_VARARGS,
   "add_warning(s)\nAdds a warning note to indicate a non-fatal problem loading a file"},
  {"add_frame",FORWARDER(imageprops_add_frame), METH_VARARGS, "add_frame([width,height]) -> frame\nAppends a frame with the optionally specified width and height. Returns a FrameProps object referring to the new frame."},
  {"get_frame",(PyCFunction)imageprops_get_frame, METH_VARARGS, "get_frame(i) -> frame\nReturns the i:th frame"},
  {"set_error",FORWARDER(imageprops_set_error), METH_VARARGS, "set_error(s)\nSet an error state described by the string s for this ImageProps. This will inhibit opening a new image tab."},
  {0,0,0,0}  // Sentinel
};

// Python standard methods follow...
static PyObject* imageprops_new(PyTypeObject* type, PyObject*, PyObject*){
  imagePropsObject* self;
  self = (imagePropsObject*)type->tp_alloc(type, 0);
  return (PyObject *)self;
}

static PyObject* imageprops_repr(imagePropsObject* /*self*/ ){
  return Py_BuildValue("s", "ImageProps");
}

static int imageprops_init(imagePropsObject* self, PyObject* /*args*/, PyObject* ){
  self->props = nullptr;
  PyErr_SetString(PyExc_TypeError,
    "ImageProps objects can not be created manually.");
  return faint::init_fail;
}

PyTypeObject ImagePropsType = {
  PyVarObject_HEAD_INIT(nullptr, 0)
  "ImageProps", //tp_name
  sizeof(imagePropsObject), // tp_basicsize
  0, // tp_itemsize
  0, // tp_dealloc
  0, // tp_print
  0, // tp_getattr
  0, // tp_setattr
  0, // tp_compare
  (reprfunc)imageprops_repr, // tp_repr
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
  "Used for defining an image", // tp_doc
  0, // tp_traverse
  0, // tp_clear
  0, // tp_richcompare
  0, // tp_weaklistoffset
  0, // tp_iter
  0, // tp_iternext
  image_props_methods, // tp_methods
  0, // tp_members
  0, // tp_getset
  0, // tp_base
  0, // tp_dict
  0, // tp_descr_get
  0, // tp_descr_set
  0, // tp_dictoffset
  (initproc)imageprops_init, // tp_init
  0, // tp_alloc
  imageprops_new, // tp_new
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

PyObject* pythoned( ImageProps& props ){
  imagePropsObject* py_props = (imagePropsObject*)ImagePropsType.tp_alloc(&ImagePropsType,0);
  py_props->props = &props;
  return (PyObject*)py_props;
}

} // namespace
