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

#include <algorithm>
#include "bitmap/bitmap.hh"
#include "objects/objcomposite.hh"
#include "objects/objtext.hh"
#include "python/py-include.hh"
#include "python/py-create-object.hh"
#include "python/py-frame-props.hh"
#include "python/py-tri.hh"
#include "python/py-ugly-forward.hh"
#include "python/py-util.hh"
#include "util/frame-props.hh"
#include "util/image-props.hh"

namespace faint{

BoundFrameProps::BoundFrameProps(ImageProps& in_image, FrameProps& in_frame)
  : image(in_image),
    frame(in_frame)
{}

BoundFrameProps get_cpp_object(framePropsObject* self){
  return BoundFrameProps(*self->imageProps,
    self->imageProps->GetFrame(self->frame_index));
}

FrameProps& get_frame_props(framePropsObject* self){
  return self->imageProps->GetFrame(self->frame_index);
}

bool faint_side_ok(framePropsObject* self){
  int index = self->frame_index;
  if (index < 0){
    return false;
  }
  if (self->imageProps->GetNumFrames() <= index){
    return false;
  }
  return true;
}
void show_error(framePropsObject*){
  PyErr_SetString(PyExc_ValueError, "Operation attempted on broken FrameProps");
}

template<Object* (*Creator)(PyObject*)>
static PyObject* add_object(framePropsObject* self, PyObject* args){
  Object* obj = Creator(args);
  if (obj == nullptr){
    return nullptr;
  }

  // Since Faint-Objects wrapped in a smthObject require a canvas
  // (which is not available when using Frameprops) - use identifiers
  // and forwarding functions to access the added objects instead
  // (e.g. frameprops_set_obj_tri).
  FrameProps& frame(get_frame_props(self));
  int id = frame.AddObject(obj);
  return Py_BuildValue("i", id);
}

static PyObject* frameprops_set_size(framePropsObject* self, PyObject* args){
  int w, h;
  if (!PyArg_ParseTuple(args, "ii", &w, &h)){
    return nullptr;
  }
  FrameProps& frame(get_frame_props(self));
  frame.SetSize(IntSize(w,h));
  return Py_BuildValue("");
}

static PyObject* frameprops_obj_composite(framePropsObject* self, PyObject* args){
  if (PySequence_Length(args) == 0){
    PyErr_SetString(PyExc_ValueError, "A group must contain at least one object.");
    return nullptr;
  }

  PyObject* sequence = args;

  // Unwrap if args contains a single sequence
  PyObject* unwrapped = nullptr;
  if (PySequence_Length(args) == 1){
    unwrapped = PySequence_GetItem(args, 0);
    if (!PySequence_Check(unwrapped)){
      py_xdecref(unwrapped);
    }
    else if (PySequence_Length(sequence) == 0){
      py_xdecref(sequence);
      PyErr_SetString(PyExc_ValueError,
        "A group must contain at least one object.");
      return nullptr;
    }
    else {
      sequence = unwrapped;
    }
  }
  FrameProps& frame(get_frame_props(self));
  const int n = PySequence_Length(sequence);
  objects_t objects;
  for (int i = 0; i != n; i++){
    ScopedRef object(PySequence_GetItem(sequence, i));
    if (!PyNumber_Check(*object)){
      py_xdecref(unwrapped);
      PyErr_SetString(PyExc_TypeError, "Not an object id");
      return nullptr;
    }

    FrameProps::ObjId id = (FrameProps::ObjId)PyLong_AsLong(*object);
    if (!frame.HasObject(id)){
      py_xdecref(unwrapped);
      PyErr_SetString(PyExc_ValueError, "Invalid object id");
      return nullptr;
    }
    if (!frame.IsTopLevel(id)){
      py_xdecref(unwrapped);
      PyErr_SetString(PyExc_ValueError, "Object is already in a group");
      return nullptr;
    }
    objects.push_back(frame.GetObject(id));
  }

  objects_t::iterator uniqueEnd = std::unique(begin(objects), end(objects));
  if (uniqueEnd != end(objects)){
    py_xdecref(unwrapped);
    PyErr_SetString(PyExc_ValueError, "Duplicate object identifiers specified");
    return nullptr;
  }

  // The composite owns the objects since they're not added
  // with an AddObjectCommand, but created during loading
  Object* composite = create_composite_object(objects, Ownership::OWNER);

  for (Object* obj : objects){
    frame.RemoveObject(obj);
  }

  int groupId = frame.AddObject(composite);
  py_xdecref(unwrapped);
  return Py_BuildValue("i", groupId);
}

static PyObject* frameprops_obj_ellipse(framePropsObject* self, PyObject* args){
  return add_object<ellipse_from_py_args>(self, args);
}

static PyObject* frameprops_obj_line(framePropsObject* self, PyObject* args){
  return add_object<line_from_py_args>(self, args);
}

static PyObject* frameprops_obj_path(framePropsObject* self, PyObject* args){
  return add_object<path_from_py_args>(self, args);
}

static PyObject* frameprops_obj_polygon(framePropsObject* self, PyObject* args){
  return add_object<polygon_from_py_args>(self, args);
}

static PyObject* frameprops_obj_raster(framePropsObject* self, PyObject* args){
  return add_object<raster_from_py_args>(self, args);
}

static PyObject* frameprops_obj_rect(framePropsObject* self, PyObject* args){
  return add_object<rectangle_from_py_args>(self, args);
}

static PyObject* frameprops_obj_spline(framePropsObject* self, PyObject* args){
  return add_object<spline_from_py_args>(self, args);
}

static PyObject* frameprops_obj_text(framePropsObject* self, PyObject* args){
  return add_object<text_from_py_args>(self, args);
}

static PyObject* frameprops_set_obj_tri(framePropsObject* self, PyObject* args){
  int objectId;
  triObject* tri;
  if (!PyArg_ParseTuple(args, "iO!", &objectId, &TriType, &tri)){
    return nullptr;
  }
  FrameProps& frame(get_frame_props(self));
  if (!frame.HasObject(objectId)){
    PyErr_SetString(PyExc_ValueError, "Invalid object identifier");
  }
  if (!valid(tri->tri)){
    PyErr_SetString(PyExc_ValueError, "Invalid Tri");
    return nullptr;
  }

  Object* object = frame.GetObject(objectId);
  object->SetTri(tri->tri);
  return Py_BuildValue("");
}

static PyObject* frameprops_get_obj_tri(framePropsObject* self, PyObject* args){
  int objectId;
  if (!PyArg_ParseTuple(args, "i", &objectId)){
    return nullptr;
  }

  FrameProps& frame = get_frame_props(self);
  if (!frame.HasObject(objectId)){
    PyErr_SetString(PyExc_ValueError, "Invalid object identifier");
  }

  Object* object = frame.GetObject(objectId);
  return pythoned(object->GetTri());
}

coord frameprops_get_obj_text_height(const BoundFrameProps& self, const int& objectId){
  if (!self.frame.HasObject(objectId)){
    throw ValueError("Invalid object identifier");
  }

  Object* object = self.frame.GetObject(objectId);
  ObjText* textObject = dynamic_cast<ObjText*>(object);
  if (textObject == nullptr){
    throw TypeError("Not a text object");
  }
  return textObject->BaselineOffset();
}

static void frameprops_set_background_color(const BoundFrameProps& self, const Color& color){
  self.frame.SetBackgroundColor(color);
}

static PyObject* frameprops_set_background_png_string(framePropsObject* self, PyObject* args){
  FrameProps& frame = get_frame_props(self);
  Bitmap bmp;
  if (!parse_png_bitmap(args, bmp)){
    return nullptr;
  }
  frame.SetBitmap(bmp);
  return Py_BuildValue("");
}

static PyObject* frameprops_set_bitmap(framePropsObject* self, PyObject* args){
  FrameProps& frame = get_frame_props(self);
  PyObject* raw_bmp;
  if (!PyArg_ParseTuple(args, "O", &raw_bmp)){
    return nullptr;
  }

  const Bitmap* bmp = as_Bitmap(raw_bmp);
  if (bmp == nullptr){
    return nullptr;
  }
  if (!bitmap_ok(*bmp)){
    PyErr_SetString(PyExc_ValueError, "Invalid Bitmap");
    return nullptr;
  }
  frame.SetBitmap(*bmp);
  return Py_BuildValue("");
}

static void frameprops_set_error(const BoundFrameProps& self, const utf8_string& err){
  self.image.SetError(err);
}

static void frameprops_add_warning(const BoundFrameProps& self, const utf8_string& warn){
  self.image.AddWarning(warn);
}

// Method table for Python-FrameProperties interface (excluding
// standard functions like del, repr and str)
static PyMethodDef frame_props_methods[] = {
  {"add_warning",FORWARDER(frameprops_add_warning), METH_VARARGS,
   "add_warning(s)\nAdds a warning note to indicate a non-fatal problem loading a file"},
  {"set_size",(PyCFunction)frameprops_set_size, METH_VARARGS, "set_size(w,h)\nSpecifies the image size."},
  {"Group", (PyCFunction)frameprops_obj_composite, METH_VARARGS, "Group(id1,id2,...) -> objectId\nGroups the objects with the specified ids, returns a new identifier for the group."},
  {"Ellipse",(PyCFunction)frameprops_obj_ellipse, METH_VARARGS, "Ellipse(x,y,w,h,settings) -> objectId\nCreates an ellipse object and returns its id."},
  {"Line",(PyCFunction)frameprops_obj_line, METH_VARARGS, "Line(x0,y0,x1,y1,settings) -> objectId\nCreates a line object and returns its id."},
  {"Path",(PyCFunction)frameprops_obj_path, METH_VARARGS, "Path(s,settings) -> objectId\nCreates a path object from the svg-path specification s, returns the id of the path."},
  {"Polygon",(PyCFunction)frameprops_obj_polygon, METH_VARARGS, "Polygon((x0,y0,x1,y1,...,xn,yn),settings) -> objectId\nCreates a polygon from the sequence of coordinates and returns the polygon id."},
  {"Raster",(PyCFunction)frameprops_obj_raster, METH_VARARGS, "Raster(x,y,w,h,bmp_type,bmp_string,settings)-> objectId\nCreates a raster object and returns its id.\nbmp_type must be 'jpeg' or 'png'.\nbmp_string must be a string containing the encoded bitmap data, of the specified bmp_type."},
  {"Rect",(PyCFunction)frameprops_obj_rect, METH_VARARGS, "Rect((x,y,w,h),settings) -> objectId\nCreates a rectangle object and returns its id."},
  {"Spline",(PyCFunction)frameprops_obj_spline, METH_VARARGS, "Spline((x0,y0,x1,y1,...,xn,yn),settings) -> objectId\nCreates a spline object and returns its id."},
  {"Text",(PyCFunction)frameprops_obj_text, METH_VARARGS, "Text((x,y,w,h),text,settings) -> objectId\nCreates a text object and returns its id."},
  {"set_obj_tri",(PyCFunction)frameprops_set_obj_tri, METH_VARARGS, "set_obj_tri(objectId, tri) Sets the Tri for the object specified by objectId"},
  {"get_obj_tri",(PyCFunction)frameprops_get_obj_tri, METH_VARARGS, "get_obj_tri(objectId) -> Tri\nReturns the Tri for the specified object"},
  {"get_obj_text_height",FORWARDER(frameprops_get_obj_text_height), METH_VARARGS, "get_obj_text_height(objectId) -> Returns the height of the rows of the text object specified by objectId"},
  {"set_background_color",FORWARDER(frameprops_set_background_color), METH_VARARGS, "Specify the background color."},
  {"set_background_png_string",(PyCFunction)frameprops_set_background_png_string, METH_VARARGS, "Specify the background image."},
  {"set_bitmap",(PyCFunction)frameprops_set_bitmap, METH_VARARGS,
   "set_bitmap(bmp)\nSpecify the bitmap to use as background image."},
  {"set_error",FORWARDER(frameprops_set_error), METH_VARARGS, "set_error(s)\nSet an error state described by the string s for the ImageProps containing this FrameProps. This will inhibit opening a new image tab."},
  {0,0,0,0}  // Sentinel
};

// Python standard methods follow...
static PyObject* frameprops_new(PyTypeObject* type, PyObject*, PyObject*){
  framePropsObject* self;
  self = (framePropsObject*)type->tp_alloc(type, 0);
  return (PyObject *)self;
}

static PyObject* frameprops_repr(framePropsObject* /*self*/){
  return Py_BuildValue("s", "FrameProps");
}

static int frameprops_init(framePropsObject* self, PyObject* /*args*/, PyObject*){
  self->imageProps = nullptr;
  PyErr_SetString(PyExc_TypeError,
    "FrameProps can only be initialized via ImageProps.add_frame.");
  return init_fail;
}

PyTypeObject FramePropsType = {
  PyVarObject_HEAD_INIT(nullptr, 0)
  "FrameProps", //tp_name
  sizeof(framePropsObject), // tp_basicsize
  0, // tp_itemsize
  0, // tp_dealloc
  0, // tp_print
  0, // tp_getattr
  0, // tp_setattr
  0, // tp_compare
  (reprfunc)frameprops_repr, // tp_repr
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
  frame_props_methods, // tp_methods
  0, // tp_members
  0, // tp_getset
  0, // tp_base
  0, // tp_dict
  0, // tp_descr_get
  0, // tp_descr_set
  0, // tp_dictoffset
  (initproc)frameprops_init, // tp_init
  0, // tp_alloc
  frameprops_new, // tp_new
  0, // tp_free
  0, // tp_is_gc
  0, // tp_bases
  0, // tp_mro
  0, // tp_cache
  0, // tp_subclasses
  0, // tp_weaklist
  0, // tp_del
  0, // tp_version_tag
  0 // tp_finalize
};

} // namespace
