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

#include "bitmap/bitmap.hh"
#include "geo/arc.hh"
#include "geo/intrect.hh"
#include "geo/line.hh"
#include "geo/rect.hh"
#include "python/py-function-error.hh"
#include "python/py-include.hh"
#include "python/py-grid.hh"
#include "python/py-util.hh"
#include "python/py-parse.hh"
#include "python/py-settings.hh"
#include "python/py-something.hh"
#include "python/py-tri.hh"
#include "text/split-string.hh" // for line_t
#include "util/file-path.hh"
#include "util/gradient.hh"
#include "util/grid.hh"
#include "util/keycode.hh"
#include "util/paint.hh"
#include "util/settings.hh"

namespace faint{

bool parse_int(PyObject* args, int n, int* value){
  PyObject* obj = PySequence_GetItem(args, n);
  long temp = PyLong_AsLong(obj);
  py_xdecref(obj);
  if (temp == -1 && PyErr_Occurred()){
    return false;
  }
  *value = temp;
  return true;
}

bool parse_bool(PyObject* args, int n, bool* value){
  PyObject* obj = PySequence_GetItem(args, n);
  *value = (1 == PyObject_IsTrue(obj));
  py_xdecref(obj);
  return true;
}

bool parse_index_t(PyObject* args, int n, index_t* value){
  PyObject* obj = PySequence_GetItem(args, n);
  long temp = PyLong_AsLong(obj);
  py_xdecref(obj);
  if (temp == -1 && PyErr_Occurred()){
    return false;
  }
  if (temp < 0){
    throw ValueError("Negative index specified.");
  }
  *value = index_t(temp);
  return true;
}

bool parse_coord(PyObject* args, int n, coord* value){
  if (PySequence_Check(args)){
    PyObject* obj = PySequence_GetItem(args, n);
    coord temp = PyFloat_AsDouble(obj);
    py_xdecref(obj);
    if (PyErr_Occurred()){
      return false;
    }
    *value = temp;
    return true;
  }
  else if (PyNumber_Check(args)){
    // Fixme: This is a weird special case to support setters. Instead
    // figure out when an arg-list is being parsed, vs. a type.
    *value = PyFloat_AsDouble(args);
    if (*value == -1.0){
      if (PyErr_Occurred()){
        return false;
      }
    }
    return true;
  }
  else{
    throw ValueError("Float argument required.");
  }
}

bool parse_bytes(PyObject* args, int, std::string* value){
  char* str = PyBytes_AsString(args);
  if (str == nullptr){
    // PyBytes_AsString will have raised TypeError
    return false;
  }
  *value = std::string(str);
  return true;
}

bool parse_unicode_string(PyObject* args, utf8_string* value){
  // Fixme: this looks familiar, duplicate function?

  if (PyUnicode_Check(args)){
    ScopedRef utf8(PyUnicode_AsUTF8String(args));
    if (*utf8 == nullptr){
      // PyUnicode_AsUTF8String will have raised an error
      return false;
    }
    const char* str = PyBytes_AsString(*utf8);
    if (str == nullptr){
      // PyString_AsString will have raised TypeError
      return false;
    }
    *value = utf8_string(str);
    return true;
  }
  throw TypeError("Not a string object");
}

bool parse_flat(Grid& grid, PyObject* args, int& n, int len){
  if (len - n < 1){
    throw ValueError("Too few arguments for parsing grid");
  }
  if (!PyObject_IsInstance(args, (PyObject*)&GridType)){
    throw TypeError("Argument must be a Grid");
  }
  Optional<Grid> maybeGrid = get_grid((gridObject*)args);
  if (!maybeGrid.IsSet()){
    return false;
  }
  grid = maybeGrid.Get();
  n += 1;
  return true;
}

bool parse_flat(bool& value, PyObject* args, int& n, int len){
  if (len - n < 1){
    throw ValueError("Too few arguments for parsing boolean");
  }

  if (PySequence_Check(args)){
    if (!parse_bool(args, n, &value)){
      return false;
    }
    n += 1;
    return true;
  }
  else{
    value = (1 == PyObject_IsTrue(args));
    n += 1;
    return true;
  }
}

bool parse_flat(int& value, PyObject* args, int& n, int len){
  if (len - n < 1){
    throw ValueError("Too few arguments for parsing integer");
  }

  if (PySequence_Check(args)){
    if (!parse_int(args, n, &value)){
      return false;
    }
    n += 1;
    return true;
  }

  long temp = PyLong_AsLong(args);
  if (temp == -1 && PyErr_Occurred()){
    return false;
  }
  value = temp;
  n += 1;
  return true;
}

bool parse_flat(index_t& value, PyObject* args, int& n, int len){
  if (len -n < 1){
    throw ValueError("Too few arguments for parsing index");
  }

  if (parse_index_t(args, n, &value)){
    n += 1;
    return true;
  }
  return false;
}

bool parse_flat(coord& value, PyObject* args, int& n, int len){
  if (len - n < 1){
    throw ValueError("Too few arguments for parsing float");
  }

  if (parse_coord(args, n, &value)){
    n += 1;
    return true;
  }
  return false;
}

bool parse_flat(IntSize& size, PyObject* args, int& n, int len){
  if (len - n < 2){
    throw ValueError("Too few arguments for parsing integer size");
  }

  int w, h;
  if (!parse_int(args, n, &w) ||!parse_int(args, n+1, &h)){
    return false;
  }
  if (w <= 0 || h <= 0){
    throw ValueError("Size must be positive");
  }
  size.w = w;
  size.h = h;
  return true;
}

bool parse_flat(IntRect& r, PyObject* args, int& n, int len){
  if (len - n < 4){
    throw ValueError("Too few arguments for parsing IntRect");
  }
  int x, y, w, h;
  if (!parse_int(args, n, &x) ||
    !parse_int(args, n + 1, &y) ||
    !parse_int(args, n + 2, &w) ||
    !parse_int(args, n + 3, &h)){
    // Note: PyErr already set by parse_int
    return false;
  }
  n += 4;
  r.x = x;
  r.y = y;
  r.w = w;
  r.h = h;
  return true;
}

bool parse_flat(Tri& tri, PyObject* args, int& n, int len){
  if (len - n < 1){
    throw ValueError("Too few arguments for parsing Tri");
  }

  ScopedRef ref(PySequence_GetItem(args, n));
  if (!PyObject_IsInstance(*ref, (PyObject*)&TriType)){
    throw TypeError("Not a Tri object");
  }

  tri = ((triObject*)(*ref))->tri;
  n += 1;
  return true;
}

bool parse_flat(Color& color, PyObject* args, int& n, int len){
  const int items = len - n;
  int readItems = 0;
  if (items < 3){
    throw ValueError("Too few arguments for parsing color");
  }

  int r, g, b;
  int a = 255;
  if (!parse_int(args, n, &r) ||
    !parse_int(args, n + 1, &g) ||
    !parse_int(args, n + 2, &b)){
    return false;
  }
  readItems += 3;

  if (items >= 4){
    if (!parse_int(args, n + 3, &a)){
      return false;
    }
    readItems += 1;
  }
  if (!valid_color(r,g,b,a)){
    throw ValueError("Invalid color");
  }

  n += readItems;
  color = color_from_ints(r,g,b,a);
  return true;
}

bool parse_flat(Rect& r, PyObject* args, int& n, int len){
  if (len - n < 4){
    throw ValueError("Too few arguments for parsing Rect");
  }
  coord x, y, w, h;
  if (!parse_coord(args, n, &x) ||
    !parse_coord(args, n + 1, &y) ||
    !parse_coord(args, n + 2, &w) ||
    !parse_coord(args, n + 3, &h)){
    // Note: PyErr already set by parse_coord
    return false;
  }
  n += 4;
  r.x = x;
  r.y = y;
  r.w = w;
  r.h = h;
  return true;
}

bool parse_flat(Paint& src, PyObject* args, int& n, int len){
  if (len - n <= 0){
    throw ValueError("Too few arguments for parsing Paint");
  }

  if (len - n > 1){
    Color c;
    if (parse_flat(c, args, n, len)){
      src = Paint(c);
      return true;
    }
    return false;
  }
  else if (len - n == 1){
    ScopedRef obj(PySequence_GetItem(args, n));
    Optional<Gradient> gradient = as_Gradient(*obj);
    if (gradient.IsSet()){
      src = Paint(gradient.Get());
      n += 1;
      return true;
    }

    PyErr_Clear();
    Pattern* pattern = as_Pattern(*obj);
    if (pattern != nullptr){
      src = Paint(*pattern);
      n+= 1;
      return true;
    }
  }
  return false;
}

bool parse_flat(IntPoint& pt, PyObject* args, int& n, int len){
  if (len - n < 2){
    throw ValueError("Too few arguments for parsing IntPoint");
  }

  int x, y;
  if (!parse_int(args, n, &x) ||
    !parse_int(args,n+1, &y)){
    return false;
  }
  pt.x = x;
  pt.y = y;
  return true;
}

bool parse_flat(ColorStop& stop, PyObject* args, int& n, int len){
  double offset;
  int r,g,b;
  int a = 255;
  if (len - n < 2){
    throw ValueError("Too few arguments for color stop");
  }

  if (!PyArg_ParseTuple(args + n, "d(iii)", &offset, &r, &g, &b)){
    PyErr_Clear();
    if (!PyArg_ParseTuple(args + n, "d(iiii)", &offset, &r, &g, &b, &a)){
      PyErr_Clear();
      throw ValueError("Invalid color stop");
    }
  }

  if (offset < 0.0 || 1.0 < offset){
    throw ValueError("Offset must be in range 0.0-1.0");
  }
  if (invalid_color(r,g,b,a)){
    throw ValueError("Invalid color specified");
  }
  stop = ColorStop(color_from_ints(r,g,b,a), offset);
  return true;
}

bool parse_flat(LineSegment& line, PyObject* args, int& n, int len){
  if (len - n < 4){
    throw ValueError("Too few arguments for parsing line segment");
  }

  coord x0;
  coord y0;
  coord x1;
  coord y1;
  Point p1;
  if (!parse_coord(args, n, &x0) ||
    !parse_coord(args, n + 1, &y0) ||
    !parse_coord(args, n + 2, &x1) ||
    !parse_coord(args, n + 3, &y1)){

    throw ValueError("Line argument requires four coordinates");
  }

  n += 4;
  line.p0.x = x0;
  line.p0.y = y0;
  line.p1.x = x1;
  line.p1.y = y1;
  return true;
}

bool parse_flat(Point& pt, PyObject* args, int& n, int len){
  if (len - n < 2){
    throw ValueError("Too few arguments for parsing IntPoint");
  }

  coord x, y;
  if (!parse_coord(args, n, &x) ||
    !parse_coord(args,n+1, &y)){
    return false;
  }
  pt.x = x;
  pt.y = y;
  return true;
}

bool parse_flat(ColRGB& color, PyObject* args, int& n, int len){
  if (len - n < 3){
    throw ValueError("Too few arguments for parsing rgb-color");
  }

  int r, g, b;
  if (!parse_int(args, n, &r) ||
    !parse_int(args, n + 1, &g) ||
    !parse_int(args, n + 2, &b)){

    return false;
  }

  if (!valid_color(r,g,b)){
    throw ValueError("Color component outside valid range");
  }

  color = rgb_from_ints(r,g,b);
  return true;
}

bool parse_flat(std::string& value, PyObject* args, int& n, int len){
  if (len - n < 1){
    throw ValueError("Too few arguments for parsing byte string");
  }

  if (parse_bytes(args, n, &value)){
    n += 1;
    return true;
  }
  return false;
}

bool parse_flat(utf8_string& value, PyObject* args, int& n, int /*len*/){
  if (n != 0){
    throw ValueError("Flat string parse not starting at 0?");
  }
  if (parse_unicode_string(args, &value)){
    n += 1;
    return true;
  }
  return false;
}

bool parse_flat(Settings& s, PyObject* args, int& n, int len){
  if (len - n < 1){
    throw ValueError("Too few arguments for parsing Settings");
  }

  ScopedRef ref(PySequence_GetItem(args, n));
  if (!PyObject_IsInstance(*ref, (PyObject*)&SettingsType)){
    throw TypeError("Not a Settings object");
  }

  s = *(((settingsObject*)(*ref))->settings);
  n += 1;
  return true;
}

bool parse_flat(Bitmap& bmp, PyObject* args, int& n, int len){
  if (len - n < 1){
    throw ValueError("Too few arguments for parsing Bitmap");
  }

  ScopedRef ref(PySequence_GetItem(args, n));
  Bitmap* tempBmp = as_Bitmap(*ref);
  if (tempBmp == nullptr){
    return nullptr;
  }
  n += 1;

  bmp = *tempBmp; // Fixme: Unnecessary copy
  return true;
}

bool parse_flat(AngleSpan& span, PyObject* args, int& n, int len){
  if (len - n < 2){
    throw ValueError("Too few arguments for parsing angle span");
  }
  radian a1;
  radian a2;
  if (!parse_coord(args, n, &a1) || !parse_coord(args, n + 1, &a2)){
    return false;
  }
  n += 2;
  span.start = Angle::Rad(a1);
  span.stop = Angle::Rad(a2);
  return true;
}

bool parse_flat(KeyPress& value, PyObject* args, int& n, int len){
  if (len - n < 2){
    throw ValueError("Too few arguments for parsing KeyPress");
  }

  int keyCode, modifier;
  if (!parse_int(args, n, &keyCode) || !parse_int(args, n + 1, &modifier))
  {
    return false;
  }

  // Fixme: Add sanity check for code and modifiers
  n += 2;
  value = KeyPress(Mod::Create(modifier), Key(keyCode));
  return true;
}

PyObject* build_result(Canvas& canvas){
  return pythoned(canvas);
}

PyObject* build_result(const Bitmap& bmp){
  return build_bitmap(bmp);
}

PyObject* build_result(const Paint& src){
  return build_paint(src);
}

PyObject* build_result(const Point& pos){
  return Py_BuildValue("dd", pos.x, pos.y);
}

PyObject* build_result(const ColorStop& stop){
  Color c(stop.GetColor());
  double offset(stop.GetOffset());
  return Py_BuildValue("d(iiii)", offset, c.r, c.g, c.b, c.a);
}

PyObject* build_result(const CanvasGrid& grid){
  return py_grid_canvas(grid.canvas);
}

PyObject* build_result(const line_t& line){
  int hardBreak = line.hardBreak ? 1 : 0;
  return Py_BuildValue("(iOd)", hardBreak, build_result(line.text),
    line.width);
}

PyObject* build_result(const Angle& angle){
  return build_result(angle.Rad());
}

PyObject* build_result(const IntRect& rect){
  return Py_BuildValue("iiii",rect.x, rect.y, rect.w, rect.h);
}

PyObject* build_result(const Rect& rect){
  return Py_BuildValue("dddd",rect.x, rect.y, rect.w, rect.h);
}

PyObject* build_result(const Settings& s){
  return pythoned(s);
}


PyObject* build_result(const index_t& index){
  return Py_BuildValue("d", index.Get());
}

PyObject* build_result(const IntPoint& pos){
  return Py_BuildValue("ii", pos.x, pos.y);
}

PyObject* build_result(const IntSize& size){
  return Py_BuildValue("ii", size.w, size.h);
}

PyObject* build_result(const Tri& tri){
  return pythoned(tri);
}

PyObject* build_result(bool value){
  return value ? Py_True : Py_False;
}

PyObject* build_result(const Color& color){
  return build_color_tuple(color);
}

PyObject* build_result(const utf8_string& s){
  return build_unicode(s);
}

PyObject* build_result(const std::string& str){
  return PyBytes_FromStringAndSize(str.c_str(), resigned(str.size()));
}

PyObject* build_result(const FilePath& filePath){
  return build_unicode(filePath.Str());
}

PyObject* build_result(const DirPath& dirPath){
  return build_unicode(dirPath.Str());
}

PyObject* build_result(const BoundObject& obj){
  return pythoned(obj.obj, obj.canvas, obj.frameId);
}

} // namespace
