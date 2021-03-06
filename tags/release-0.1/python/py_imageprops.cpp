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
#include "pythoninclude.hh"
#include "py_imageprops.hh"
#include "py_create_object.hh"
#include "py_tri.hh"
#include "objects/objcomposite.hh"
#include "objects/objtext.hh"
#include "util/util.hh"

template<Object* (*Creator)(PyObject*)>
static PyObject* add_object(imagePropsObject* self, PyObject* args ){
  Object* obj = Creator( args );
  if ( obj == 0 ){
    return 0;
  }

  // Since Faint-Objects wrapped in a smthObject require a canvas
  // (which is not available when using ImageProps) - use identifiers
  // and forwarding functions to access the added objects instead
  // (e.g. imageprops_set_obj_tri).
  int id = self->props->AddObject(obj);
  return Py_BuildValue("i", id);
}

static PyObject* imageprops_set_size(imagePropsObject* self, PyObject* args ){
  int w, h;
  if ( !PyArg_ParseTuple(args, "ii", &w, &h ) ){
    return 0;
  }
  self->props->SetSize(IntSize(w,h));
  return Py_BuildValue("");
}

static PyObject* imageprops_obj_composite(imagePropsObject* self, PyObject* args ){
  if ( PySequence_Length(args) == 0 ){
    PyErr_SetString( PyExc_ValueError, "A group must contain at least one object." );
    return 0;
  }

  PyObject* sequence = args; // Fixme: Do like in py_canvas for niceness.
  const int n = PySequence_Length(sequence);
  std::vector<Object*> objects;
  for ( int i = 0; i != n; i++ ){
    PyObject* object = PySequence_GetItem( sequence, i );
    if ( !PyNumber_Check( object ) ){
      PyErr_SetString(PyExc_ValueError, "Not an object id"); // Fixme: Better error message
      return 0;
    }

    ImageProps::ObjId id = (ImageProps::ObjId)PyLong_AsLong(object);
    if ( !self->props->HasObject(id) ){
      PyErr_SetString(PyExc_ValueError, "Invalid object id"); // Fixme: Better error message
      return 0;
    }
    if ( !self->props->IsTopLevel(id) ){
      PyErr_SetString(PyExc_ValueError, "Object is already in a group"); // Fixme: Better error message
      return 0;
    }
    objects.push_back( self->props->GetObject( id ) );
  }

  std::vector<Object*>::iterator uniqueEnd = std::unique( objects.begin(), objects.end() );
  if ( uniqueEnd != objects.end() ){
    PyErr_SetString(PyExc_ValueError, "Duplicate object identifiers specified");
    return 0;
  }

  // The composite owns the objects since they're not added
  // with an AddObjectCommand, but created during loading
  ObjComposite* composite = new ObjComposite(objects, OWNER);

  for ( size_t i = 0; i != objects.size(); i++ ){
    self->props->RemoveObject(objects[i]);
  }

  int groupId = self->props->AddObject( composite );
  return Py_BuildValue("i", groupId );
}

static PyObject* imageprops_obj_ellipse(imagePropsObject* self, PyObject* args ){
  return add_object<CreateEllipse>(self, args );
}

static PyObject* imageprops_obj_line(imagePropsObject* self, PyObject* args ){
  return add_object<CreateLine>(self, args);
}

static PyObject* imageprops_obj_path(imagePropsObject* self, PyObject* args ){
  return add_object<CreatePath>(self, args);
}

static PyObject* imageprops_obj_polygon(imagePropsObject* self, PyObject* args ){
  return add_object<CreatePolygon>(self, args);
}

static PyObject* imageprops_obj_raster(imagePropsObject* self, PyObject* args ){
  return add_object<CreateRaster>(self, args);
}

static PyObject* imageprops_obj_rect(imagePropsObject* self, PyObject* args ){
  return add_object<CreateRectangle>(self, args);
}

static PyObject* imageprops_obj_spline(imagePropsObject* self, PyObject* args ){
  return add_object<CreateSpline>(self, args);
}

static PyObject* imageprops_obj_text(imagePropsObject* self, PyObject* args ){
  return add_object<CreateText>(self, args);
}

static PyObject* imageprops_set_obj_tri(imagePropsObject* self, PyObject* args ){
  int objectId;
  triObject* tri;
  if ( !PyArg_ParseTuple( args, "iO!", &objectId, &TriType, &tri ) ){
    return 0;
  }

  if ( !self->props->HasObject(objectId ) ){
    PyErr_SetString(PyExc_ValueError, "Invalid object identifier");
  }

  Object* object = self->props->GetObject(objectId);
  object->SetTri(tri->tri);
  return Py_BuildValue("");
}

static PyObject* imageprops_get_obj_tri(imagePropsObject* self, PyObject* args ){
  int objectId;
  if ( !PyArg_ParseTuple( args, "i", &objectId ) ){
    return 0;
  }

  if ( !self->props->HasObject(objectId) ){
    PyErr_SetString(PyExc_ValueError, "Invalid object identifier");
  }

  Object* object = self->props->GetObject(objectId);
  return Pythoned(object->GetTri());
}

static PyObject* imageprops_get_obj_text_height( imagePropsObject* self, PyObject* args ){
  int objectId;
  if ( !PyArg_ParseTuple( args, "i", &objectId ) ){
    return 0;
  }

  if ( !self->props->HasObject(objectId) ){
    PyErr_SetString(PyExc_ValueError, "Invalid object identifier");
    return 0;
  }

  Object* object = self->props->GetObject(objectId);
  ObjText* textObject = dynamic_cast<ObjText*>(object);
  if ( textObject == 0 ){
    PyErr_SetString(PyExc_ValueError, "Not a text object");
    return 0;
  }
  return Py_BuildValue("d", textObject->RowHeight() );
}

static PyObject* imageprops_obj_text_anchor_middle( imagePropsObject* self, PyObject* args ){
  int objectId;
  if ( !PyArg_ParseTuple( args, "i", &objectId ) ){
    return 0;
  }

  if ( !self->props->HasObject(objectId) ){
    PyErr_SetString(PyExc_ValueError, "Invalid object identifier");
    return 0;
  }

  Object* object = self->props->GetObject(objectId);
  ObjText* textObject = dynamic_cast<ObjText*>(object);
  if ( textObject == 0 ){
    PyErr_SetString(PyExc_ValueError, "Not a text object");
    return 0;
  }
  textObject->AnchorMiddle();
  return Py_BuildValue("");
}

static PyObject* imageprops_set_background_png_string( imagePropsObject* self, PyObject* args ){  
  if ( PySequence_Length(args) != 1 ){
    PyErr_SetString( PyExc_ValueError, "A single string argument required.");
    return 0;
  }
  PyObject* pngStrPy = PySequence_GetItem( args, 0 );
  if (!PyString_Check( pngStrPy ) ){
    PyErr_SetString( PyExc_ValueError, "Invalid png-string.");
    return 0;
  }
  
  Py_ssize_t len = PyString_Size( pngStrPy );
  faint::Bitmap bmp( from_png( PyString_AsString(pngStrPy), len ) );
  self->props->SetBitmap(bmp);  
  return Py_BuildValue("");
}

static PyObject* imageprops_set_error( imagePropsObject* self, PyObject* args ){
  char* errorStr;
  if ( !PyArg_ParseTuple( args, "s", &errorStr ) ){
    return 0;
  }

  self->props->SetError(errorStr);
  return Py_BuildValue("");
}


// Method table for Python-ImageProperties interface (excluding
// standard functions like del, repr and str)
static PyMethodDef imageprops_methods[] = {
  {"Group", (PyCFunction)imageprops_obj_composite, METH_VARARGS, "Fixme"},
  {"set_size",(PyCFunction)imageprops_set_size, METH_VARARGS, "Fixme"},
  {"Ellipse",(PyCFunction)imageprops_obj_ellipse, METH_VARARGS, "Fixme"},
  {"Line",(PyCFunction)imageprops_obj_line, METH_VARARGS, "Fixme"},
  {"Path",(PyCFunction)imageprops_obj_path, METH_VARARGS, "Fixme"},
  {"Polygon",(PyCFunction)imageprops_obj_polygon, METH_VARARGS, "Fixme"},
  {"Raster",(PyCFunction)imageprops_obj_raster, METH_VARARGS, "Fixme"},
  {"Rect",(PyCFunction)imageprops_obj_rect, METH_VARARGS, "Fixme"},
  {"Spline",(PyCFunction)imageprops_obj_spline, METH_VARARGS, "Fixme"},
  {"Text",(PyCFunction)imageprops_obj_text, METH_VARARGS, "Fixme"},
  {"set_obj_tri",(PyCFunction)imageprops_set_obj_tri, METH_VARARGS, "Fixme"},
  {"get_obj_tri",(PyCFunction)imageprops_get_obj_tri, METH_VARARGS, "Fixme"},
  {"get_obj_text_height",(PyCFunction)imageprops_get_obj_text_height, METH_VARARGS, "Fixme"},
  {"obj_text_anchor_middle",(PyCFunction)imageprops_obj_text_anchor_middle, METH_VARARGS, "Fixme"},
  {"set_background_png_string",(PyCFunction)imageprops_set_background_png_string, METH_VARARGS, "Fixme"},
  {"set_error",(PyCFunction)imageprops_set_error, METH_VARARGS, "Fixme"},
  {0,0,0,0}  // Sentinel
};

// Python standard methods follow...
static PyObject* imageprops_new(PyTypeObject* type, PyObject*, PyObject*){
  imagePropsObject* self;
  self = (imagePropsObject*)type->tp_alloc(type, 0);
  return (PyObject *)self;
}

static PyObject* imageprops_repr(imagePropsObject* self ){
  return Py_BuildValue("s", "ImageProps");
}

static int imageprops_init(imagePropsObject* self, PyObject* args, PyObject* ){
  self->props = 0;
  return 0;
}

PyTypeObject ImagePropsType = {
  PyObject_HEAD_INIT(NULL)
  0, // ob_size
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
  "Just some props", // tp_doc fixme
  0, // tp_traverse
  0, // tp_clear
  0, // tp_richcompare
  0, // tp_weaklistoffset
  0, // tp_iter
  0, // tp_iternext
  imageprops_methods, // tp_methods
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
  0 // tp_version_tag
};

PyObject* Pythoned( ImageProps& props ){
  imagePropsObject* py_props = (imagePropsObject*)ImagePropsType.tp_alloc(&ImagePropsType,0);
  py_props->props = &props;
  return (PyObject*)py_props;
}
