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

#include "geo/geo-func.hh"
#include "geo/points.hh"
#include "geo/rect.hh"
#include "geo/size.hh"
#include "objects/objellipse.hh"
#include "objects/objline.hh"
#include "objects/objpath.hh"
#include "objects/objpolygon.hh"
#include "objects/objraster.hh"
#include "objects/objrectangle.hh"
#include "objects/objspline.hh"
#include "objects/objtext.hh"
#include "python/py-create-object.hh"
#include "python/py-settings.hh"
#include "python/py-util.hh"
#include "util/convert-wx.hh"
#include "util/serialize-bitmap.hh"
#include "util/setting-util.hh"

namespace faint{

Object* ellipse_from_py_args(PyObject* args){
  coord x, y, w, h;
  settingsObject* pySettings=0; // afaik, O! does not increase ref-count
  if (! PyArg_ParseTuple(args, "(dddd)O!", &x, &y, &w, &h, &SettingsType, &pySettings)){
    return nullptr;
  }
  Settings ellipseSettings(default_ellipse_settings());
  ellipseSettings.Update(*(pySettings->settings));
  return create_ellipse_object(tri_from_rect(Rect(Point(x,y),Size(w,h))), ellipseSettings);
}

static Object* polyline_from_py_args(PyObject* args){
  PyObject* sequence = nullptr;
  settingsObject* pySettings = nullptr;
  if (!PyArg_ParseTuple(args, "OO!", &sequence, &SettingsType, &pySettings)){
    return nullptr;
  }
  if (!PySequence_Check(sequence)){
    PyErr_SetString(PyExc_TypeError, "Line expects a sequence of coordinates.");
    return nullptr;
  }
  const int n = PySequence_Length(sequence);
  if (n <= 0){
    PyErr_SetString(PyExc_TypeError, "No parameters passed.\nLine requires coordinate parameters.");
    return nullptr;
  }
  if (n % 2 != 0){
    PyErr_SetString(PyExc_ValueError, "Number of parameters is not even.");
    return nullptr;
  }

  Points points;
  for (int i = 0; i != n; i+=2){
    PyObject* xItem = PySequence_GetItem(sequence, i);
    if (xItem == nullptr){
      PyErr_SetString(PyExc_TypeError, "Parameter error");
      return nullptr;
    }

    if (!PyNumber_Check(xItem)){
      py_xdecref(xItem);
      PyErr_SetString(PyExc_TypeError, "Non-numeric parameter");
      return nullptr;
    }
    coord x = PyFloat_AsDouble(xItem);
    py_xdecref(xItem);

    PyObject* yItem = PySequence_GetItem(sequence, i + 1);
    if (yItem == nullptr){
      PyErr_SetString(PyExc_TypeError, "Parameter error");
      return nullptr;
    }
    if (!PyNumber_Check(yItem)){
      py_xdecref(yItem);
      PyErr_SetString(PyExc_TypeError, "Non-numeric parameter");
      return nullptr;
    }

    coord y = PyFloat_AsDouble(yItem);
    py_xdecref(yItem);
    points.Append(Point(x, y));
  }

  Settings lineSettings(default_line_settings());
  lineSettings.Update(*(pySettings->settings));
  return create_line_object(points, lineSettings);
}

Object* line_from_py_args(PyObject* args){
  coord x0, y0, x1, y1;
  settingsObject* pySettings = nullptr;
  if (!PyArg_ParseTuple(args, "(dddd)O!", &x0, &y0, &x1, &y1, &SettingsType, &pySettings)){
    PyErr_Clear();
    return polyline_from_py_args(args);
  }
  Settings lineSettings(default_line_settings());
  lineSettings.Update(*pySettings->settings);
  return create_line_object(Points(std::vector<Point>({Point(x0,y0),Point(x1,y1)})), lineSettings);
}

Object* path_from_py_args(PyObject* args){
  char* s = nullptr;
  settingsObject* pySettings = nullptr;
  if (!PyArg_ParseTuple(args, "sO!", &s, &SettingsType, &pySettings)){
    return nullptr;
  }

  std::vector<PathPt> points(parse_svg_path(s));
  if (points.empty()){
    PyErr_SetString(PyExc_ValueError, "Failed parsing Path Definition.");
    return nullptr;
  }
  if (points.front().IsNotMove()){
    PyErr_SetString(PyExc_ValueError, "Paths must begin with a Move-entry.");
    return nullptr;
  }

  Settings pathSettings(default_path_settings());
  pathSettings.Update(*(pySettings->settings));
  return create_path_object(Points(points), pathSettings);
}

Object* polygon_from_py_args(PyObject* args){
  PyObject* sequence = nullptr;
  settingsObject* pySettings = nullptr;
  if (!PyArg_ParseTuple(args, "OO!", &sequence, &SettingsType, &pySettings)){
    return nullptr;
  }

  if (!PySequence_Check(sequence)){
    PyErr_SetString(PyExc_TypeError, "The first argument must be a sequence");
  }

  const int n = PySequence_Length(sequence);
  if (n <= 0){
    PyErr_SetString(PyExc_TypeError, "No parameters passed.\nPolygon requires coordinate parameters.");
    return nullptr;
  }
  if (n % 2 != 0){
    PyErr_SetString(PyExc_ValueError, "Number of parameters is not even.");
    return nullptr;
  }

  Points points;
  for (int i = 0; i != n; i+=2){
    PyObject* xItem = PySequence_GetItem(sequence, i);
    if (xItem == nullptr){
      PyErr_SetString(PyExc_TypeError, "Parameter error");
      return nullptr;
    }

    if (!PyNumber_Check(xItem)){
      py_xdecref(xItem);
      PyErr_SetString(PyExc_TypeError, "Non-numeric parameter");
      return nullptr;
    }
    coord x = PyFloat_AsDouble(xItem);
    py_xdecref(xItem);

    PyObject* yItem = PySequence_GetItem(sequence, i + 1);
    if (yItem == nullptr){
      PyErr_SetString(PyExc_TypeError, "Parameter error");
      return nullptr;
    }
    if (!PyNumber_Check(yItem)){
      py_xdecref(yItem);
      PyErr_SetString(PyExc_TypeError, "Non-numeric parameter");
      return nullptr;
    }

    coord y = PyFloat_AsDouble(yItem);
    py_xdecref(yItem);
    points.Append(Point(x, y));
  }

  Settings polygonSettings(default_polygon_settings());
  polygonSettings.Update(*(pySettings->settings));
  return create_polygon_object(points, polygonSettings);
}

Object* rectangle_from_py_args(PyObject* args){
  coord x, y, w, h;
  settingsObject* pySettings=0;
  if (! PyArg_ParseTuple(args, "(dddd)O!", &x, &y, &w, &h, &SettingsType, &pySettings)){
    return nullptr;
  }
  Settings rectSettings(default_rectangle_settings());
  rectSettings.Update(*(pySettings->settings));
  return create_rectangle_object(tri_from_rect(Rect(Point(x,y),Size(w,h))), rectSettings);
}

Object* raster_from_py_args(PyObject* args){
  coord x, y, w, h;
  char* type_cstr;
  PyObject* o_data_str;
  settingsObject* pySettings=0;
  if (!PyArg_ParseTuple(args, "(dddd)sOO!", &x, &y, &w, &h, &type_cstr, &o_data_str, &SettingsType, &pySettings)){
    return nullptr;
  }

  if (!PyBytes_Check(o_data_str)){
    PyErr_SetString(PyExc_ValueError, "Invalid data string");
    return nullptr;
  }

  Settings rasterSettings(default_raster_settings());
  rasterSettings.Update(*(pySettings->settings));

  Py_ssize_t len = PyBytes_Size(o_data_str);
  const char* data_cstr = PyBytes_AsString(o_data_str);
  std::string type(type_cstr);
  Point p0(x,y);
  Size sz(w,h);
  Tri tri(p0, p0 + delta_x(sz.w), p0 + delta_y(sz.h));
  if (type == "jpeg"){
    return new ObjRaster(tri, from_jpg(data_cstr, to_size_t(len)), rasterSettings);
  }
  else if (type == "png"){
    return new ObjRaster(tri, from_png(data_cstr, to_size_t(len)), rasterSettings);
  }

  PyErr_SetString(PyExc_ValueError, "Invalid image type.");
  return nullptr;
}

Object* spline_from_py_args(PyObject* args){
  PyObject* sequence;
  settingsObject* pySettings=0;
  if (! PyArg_ParseTuple(args, "OO!", &sequence, &SettingsType, &pySettings)){
    return nullptr;
  }

  if (!PySequence_Check(sequence)){
    PyErr_SetString(PyExc_TypeError, "The first argument must be a sequence");
  }
  const int n = PySequence_Length(sequence);
  if (n <= 0){
    PyErr_SetString(PyExc_TypeError, "Empty sequence.\nSpline requires coordinate parameters.");
    return nullptr;
  }
  if (n % 2 != 0){
    PyErr_SetString(PyExc_ValueError, "Number of parameters is not even.");
    return nullptr;
  }

  Points points;
  for (int i = 0; i != n; i+=2){
    PyObject* xItem = PySequence_GetItem(sequence, i);
    if (xItem == nullptr){
      PyErr_SetString(PyExc_TypeError, "Parameter error");
      return nullptr;
    }

    if (!PyNumber_Check(xItem)){
      py_xdecref(xItem);
      PyErr_SetString(PyExc_TypeError, "Non-numeric parameter");
      return nullptr;
    }
    coord x = PyFloat_AsDouble(xItem);
    py_xdecref(xItem);

    PyObject* yItem = PySequence_GetItem(sequence, i + 1);
    if (yItem == nullptr){
      PyErr_SetString(PyExc_TypeError, "Parameter error");
      return nullptr;
    }
    if (!PyNumber_Check(yItem)){
      py_xdecref(yItem);
      PyErr_SetString(PyExc_TypeError, "Non-numeric parameter");
      return nullptr;
    }

    coord y = PyFloat_AsDouble(yItem);
    points.Append(Point(x, y));
  }

  Settings splineSettings(default_spline_settings());
  splineSettings.Update(*(pySettings->settings));
  return create_spline_object(points, splineSettings);
}

bool py_to_cpp_string(PyObject* str_py, std::string& str){
  if (PyUnicode_Check(str_py)){
    PyObject* utf8 = PyUnicode_AsEncodedString(str_py, "utf8", "strict");
    if (utf8 == nullptr){
      return false;
    }
    const char* c_str = PyBytes_AsString(utf8); // Pointer to internal data
    str = c_str;
    py_xdecref(utf8);
    return true;
  }
  PyErr_SetString(PyExc_TypeError, "Unicode string required.");
  return false;
}

Object* text_from_py_args(PyObject* args){
  coord x, y, w, h;
  settingsObject* pySettings = nullptr;
  PyObject* pystr = nullptr;
  if (! PyArg_ParseTuple(args, "(dddd)OO!", &x, &y, &w, &h, &pystr, &SettingsType, &pySettings)){
    return nullptr;
  }

  std::string str;
  bool convertOk = py_to_cpp_string(pystr, str);
  if (!convertOk){
    return nullptr;
  }

  Settings textSettings(default_text_settings());
  textSettings.Update(*(pySettings->settings));
  return new ObjText(Rect(Point(x,y),Size(w,h)), utf8_string(str), textSettings);
}

} // namespace
