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

#include <sstream>
#include "bitmap/draw.hh"
#include "geo/rect.hh"
#include "objects/objellipse.hh"
#include "objects/objpath.hh"
#include "objects/objraster.hh"
#include "objects/objspline.hh"
#include "objects/objtext.hh"
#include "text/formatting.hh"
#include "util/canvas.hh"
#include "util/image.hh"
#include "util/serialize-bitmap.hh"
#include "python/py-include.hh"
#include "python/py-frame.hh"
#include "python/py-function-error.hh"
#include "python/py-linear-gradient.hh"
#include "python/py-pattern.hh"
#include "python/py-radial-gradient.hh"
#include "python/py-util.hh"
#include "python/writable-str.hh"

namespace faint{

typedef std::is_same<coord, double> ensure_coord_double;

Bitmap* as_Bitmap(PyObject* obj){
  if (!PyObject_IsInstance(obj, (PyObject*)&BitmapType)){
    PyErr_SetString(PyExc_ValueError, "Not a Bitmap");
    return nullptr;
  }
  bitmapObject* py_bitmap = (bitmapObject*)obj;
  return py_bitmap->bmp;
}

Optional<Gradient> as_Gradient(PyObject* obj){
  if (PyObject_IsInstance(obj, (PyObject*)&LinearGradientType)){
    linearGradientObject* pyGradient = (linearGradientObject*)(obj);
    return option(Gradient(*pyGradient->gradient));
  }
  else if (PyObject_IsInstance(obj, (PyObject*)&RadialGradientType)){
    radialGradientObject* pyGradient = (radialGradientObject*)(obj);
    return option(Gradient(*pyGradient->gradient));
  }

  PyErr_SetString(PyExc_TypeError, "The argument must be a LinearGradient or a RadialGradient object");
  return no_option();
}

Optional<utf8_string> parse_py_unicode(PyObject* obj){
  ScopedRef utf8(PyUnicode_AsUTF8String(obj));
  if (*utf8 == nullptr){
    // PyString_AsString will have raised TypeError
    return no_option();
  }
  const char* str = PyBytes_AsString(*utf8);
  if (str == nullptr){
    // PyBytes_AsString will have raised TypeError
    return no_option();
  }
  return option(utf8_string(str));
}

Object* as_Object(PyObject* obj){
  smthObject* smthObj = as_smthObject(obj);
  if (smthObj == nullptr){
    return nullptr;
  }
  return smthObj->obj;
}

std::pair<Canvas*, ObjRaster*> as_ObjRaster(PyObject* obj){
  smthObject* smth = as_smthObject(obj);
  if (smth == nullptr){
    return std::make_pair<Canvas*, ObjRaster*>(0,0);
  }
  ObjRaster* raster = dynamic_cast<ObjRaster*> (smth->obj);
  if (raster == nullptr){
    PyErr_SetString(PyExc_ValueError, "Not a Raster object");
    return std::make_pair<Canvas*, ObjRaster*>(0,0);
  }
  return std::make_pair(smth->canvas, raster);
}

std::pair<Canvas*, ObjText*> as_ObjText(PyObject* obj){
  smthObject* smth = as_smthObject(obj);
  if (smth == nullptr){
    return std::make_pair<Canvas*, ObjText*>((Canvas*)0,(ObjText*)0);
  }
  ObjText* text = dynamic_cast<ObjText*> (smth->obj);
  if (text == nullptr){
    PyErr_SetString(PyExc_ValueError, "Not a Text object");
    return std::make_pair<Canvas*, ObjText*>(0,0);
  }
  return std::make_pair(smth->canvas, text);
}

Pattern* as_Pattern(PyObject* obj){
  if (!PyObject_IsInstance(obj, (PyObject*)&PatternType)){
    PyErr_SetString(PyExc_TypeError, "The argument must be a Pattern object");
    return nullptr;
  }
  patternObject* pyPattern = (patternObject*)(obj);
  return pyPattern->pattern;
}

smthObject* as_smthObject(PyObject* obj){
  if (!PyObject_IsInstance(obj, (PyObject*)&SmthType)){
    PyErr_SetString(PyExc_TypeError, "Not a Faint Object");
    return nullptr;
  }
  return (smthObject*)obj;
}

PyObject* build_bitmap(const Bitmap& bmp){
  assert(bitmap_ok(bmp));
  Bitmap* newBitmap = nullptr;
  try{
    newBitmap = new Bitmap(copy(bmp));
  }
  catch(const std::bad_alloc&){
    PyErr_SetString(PyExc_MemoryError,
      "Insufficient memory for allocating Bitmap");
    return nullptr;
  }
  bitmapObject* py_bitmap = (bitmapObject*)BitmapType.tp_alloc(&BitmapType, 0);
  if (py_bitmap == nullptr){
    return nullptr;
  }
  py_bitmap->bmp = newBitmap;
  return (PyObject*)py_bitmap;
}

PyObject* build_color_tuple(const Color& color){
  return Py_BuildValue("iiii", color.r, color.g, color.b, color.a);
}

PyObject* build_color_stop(const ColorStop& stop){
  Color c(stop.GetColor());
  double offset(stop.GetOffset());
  return Py_BuildValue("d(iiii)", offset, c.r, c.g, c.b, c.a);
}

static PyObject* build_gradient(const Gradient& gradient){
  if (gradient.IsLinear()){
    linearGradientObject* py_gradient = (linearGradientObject*)LinearGradientType.tp_alloc(&LinearGradientType, 0);
    py_gradient->gradient = new LinearGradient(gradient.GetLinear());
    return (PyObject*)py_gradient;
  }
  else if (gradient.IsRadial()){
    radialGradientObject* py_gradient = (radialGradientObject*)RadialGradientType.tp_alloc(&RadialGradientType, 0);
    py_gradient->gradient = new RadialGradient(gradient.GetRadial());
    return (PyObject*)py_gradient;
  }
  else {
    assert(false);
    return Py_BuildValue("");
  }
}

static PyObject* build_pattern(const Pattern& pattern){
  patternObject* py_pattern = (patternObject*)PatternType.tp_alloc(&PatternType, 0);
  py_pattern->pattern = new Pattern(pattern);
  return (PyObject*)py_pattern;
}

PyObject* build_paint(const Paint& src){
  return dispatch(src, build_color_tuple, build_pattern, build_gradient);
}

PyObject* build_frame(Canvas* canvas, const index_t& index){
  assert(index < canvas->GetNumFrames());
  frameObject* py_frame = (frameObject*)(FrameType.tp_alloc(&FrameType, 0));
  py_frame->canvas = canvas;
  py_frame->canvasId = canvas->GetId();
  py_frame->frameId = canvas->GetFrame(index).GetId();
  return (PyObject*)py_frame;
}


PyObject* build_object_list(const objects_t& objects, Canvas* canvas, const FrameId& frameId){
  PyObject* py_list = PyList_New(0);
  for (Object* obj : objects){
    PyList_Append(py_list, pythoned(obj, canvas, frameId));
  }
  return py_list;
}

PyObject* build_radii(const Radii& radii){
  return Py_BuildValue("dd", radii.x, radii.y);
}

PyObject* build_unicode(const utf8_string& str){
  return PyUnicode_DecodeUTF8(str.c_str(), resigned(str.bytes()), nullptr);
}

bool invalid_color(int r, int g, int b, int a){
  if (!(0 <= r && r <= 255)){
    PyErr_SetString(PyExc_ValueError, "Invalid rgba-color: Red component out of range (0-255).");
    return true;
  }
  else if (!(0 <= g && g <= 255)){
    PyErr_SetString(PyExc_ValueError, "Invalid rgba-color: Green component out of range (0-255).");
    return true;
  }
  else if (!(0 <= b && b <= 255)){
    PyErr_SetString(PyExc_ValueError, "Invalid rgba-color: Blue component out of range (0-255).");
    return true;
  }
  else if (!(0 <= a && a <= 255)){
    PyErr_SetString(PyExc_ValueError, "Invalid rgba-color: Alpha component out of range (0-255).");
    return true;
  }

  // Color is valid
  return false;
}

bool invalid_pixel_pos(int x, int y, const Bitmap& bmp){
  return invalid_pixel_pos(IntPoint(x,y), bmp);
}

bool invalid_pixel_pos(const IntPoint& pos, const Bitmap& bmp){
  if (!point_in_bitmap(bmp, pos)){
    PyErr_SetString(PyExc_ValueError, "Point outside image");
    return true;
  }
  return false;
}

bool objects_from_args(PyObject* args, const allow_empty& allowEmpty, objects_t& v){
  const int numArg = PySequence_Length(args);
  bool nestedSequence = (numArg == 1 && PySequence_Check(PySequence_GetItem(args, 0)));
  PyObject* sequence = nestedSequence ? PySequence_GetItem(args, 0) : args;
  const int n = nestedSequence ? PySequence_Length(sequence) : numArg;

  if (n == 0){
    if (nestedSequence && allowEmpty.Get()){
      return true;
    }
    PyErr_SetString(PyExc_ValueError, "No objects specified");
    return false;
  }

  for (int i = 0; i != n; i++){
    PyObject* item = PySequence_GetItem(sequence, i);
    smthObject* obj = as_smthObject(item);
    if (obj == nullptr){
      PyErr_SetString(PyExc_TypeError, "That's not a Faint object.");
      return false;
    }
    v.push_back(obj->obj);
  }
  return true;
}

utf8_string object_string(PyObject* obj){
  PyObject* pyStr = PyObject_Str(obj);
  if (pyStr == nullptr){
    return "";
  }
  PyObject* utf8 = PyUnicode_AsUTF8String(pyStr);
  assert(utf8 != nullptr);
  char* bytes = PyBytes_AsString(utf8);
  utf8_string str(bytes); // Fixme
  py_xdecref(pyStr);
  py_xdecref(utf8);
  return str;
}

bool parse_color(PyObject* args, Color& color, bool allowAlpha){
  int r = 255;
  int g = 255;
  int b = 255;
  int a = 255;

  bool ok = PyArg_ParseTuple(args, "iiii", &r, &g, &b, &a) ||
    PyArg_ParseTuple(args, "iii", &r, &g, &b) ||
    PyArg_ParseTuple(args, "(iiii)", &r, &g, &b, &a) ||
    PyArg_ParseTuple(args, "(iii)", &r, &g, &b);

  if (!ok){
    PyErr_SetString(PyExc_TypeError, "Invalid color specification. Valid formats for colors are r,g,b and r,g,b,a");
    return false;
  }
  PyErr_Clear();

  if (invalid_color(r, g, b, a)){
    return false;
  }

  if (!allowAlpha && a != 255){
    PyErr_SetString(PyExc_ValueError, "Alpha value not supported for this function");
    return false;
  }

  color = color_from_ints(r,g,b,a);
  return true;
}

bool parse_color_stop(PyObject* obj, ColorStop& stop){
  if (PySequence_Length(obj) != 2){
    PyErr_SetString(PyExc_ValueError, "Color stop must be specified using offset,(r,g,b[,a])");
    return false;
  }
  PyObject* pyOffset = PySequence_GetItem(obj, 0);
  if (!PyFloat_Check(pyOffset)){
    PyErr_SetString(PyExc_ValueError, "Color stop must start with a floating point offset");
    py_xdecref(pyOffset);
    return false;
  }
  double offset = PyFloat_AsDouble(pyOffset);
  py_xdecref(pyOffset);
  PyObject* pyColor = PySequence_GetItem(obj,1);
  Color c;
  if (!parse_color(pyColor, c)){
    py_xdecref(pyColor);
    return false;
  }
  stop = ColorStop(c, offset);
  return true;
}

bool parse_paint(PyObject* args, Paint& out){
  Color c;
  if (parse_color(args, c)){
    out = Paint(c);
    return true;
  }

  // Unwrap if single-item sequence
  PyObject* obj = nullptr;
  bool sequence = false;
  if (PySequence_Check(args)){
    sequence = true;
    if (PySequence_Length(args) != 1){
      PyErr_SetString(PyExc_ValueError, "Expected a color tuple, Gradient or Pattern");
      return false;
    }
    obj = PySequence_GetItem(args, 0);
  }
  else {
    obj = args;
  }
  PyErr_Clear();
  Optional<Gradient> gradient = as_Gradient(obj);
  Pattern* pattern = nullptr;
  if (gradient.NotSet()){
    PyErr_Clear();
    pattern = as_Pattern(obj);
  }
  if (sequence){
    py_xdecref(obj);
  }

  if (gradient.IsSet()){
    out = Paint(gradient.Get());
    return true;
  }
  else if (pattern != nullptr){
    out = Paint(*pattern);
    return true;
  }
  PyErr_SetString(PyExc_ValueError, "Expected a color tuple, Gradient or Pattern");
  return false;
}

bool parse_intpoint(PyObject* args, IntPoint* out){
  int x, y;
  if (PyArg_ParseTuple(args, "ii", &x, &y)){
    out->x = x;
    out->y = y;
    return true;
  }
  return false;
}

bool parse_objects(PyObject* args, objects_t* out){
  int len = PySequence_Length(args);
  if (len == 0){
    PyErr_SetString(PyExc_ValueError, "No objects specified.");
    return false;
  }

  if (len == 1){
    PyObject* firstArg = PySequence_GetItem(args, 0);
    if (PySequence_Check(firstArg)){
      args = firstArg;
      len = PySequence_Length(args);
      if (len == 0){
        PyErr_SetString(PyExc_ValueError, "No objects specified");
        return false;
      }
    }
  }

  std::vector<Object*> objs;
  for (int i = 0; i != len; i++){
    PyObject* pyObj = PySequence_GetItem(args, i);
    smthObject* smthObj = as_smthObject(pyObj);
    if (smthObj == nullptr){
      PyErr_SetString(PyExc_ValueError, "Not an object");
      return false;
    }
    objs.push_back(smthObj->obj);
  }
  *out = objs;
  return true;
}

bool parse_png_bitmap(PyObject* args, Bitmap& out){
  if (PySequence_Length(args) != 1){
    PyErr_SetString(PyExc_TypeError, "A single string argument required.");
    return nullptr;
  }

  PyObject* pngStrPy = PySequence_GetItem(args, 0);
  if (!PyBytes_Check(pngStrPy)){
    py_xdecref(pngStrPy);
    PyErr_SetString(PyExc_TypeError, "Invalid png-string.");
    return false;
  }

  Py_ssize_t len = PyBytes_Size(pngStrPy);
  Bitmap bmp(from_png(PyBytes_AsString(pngStrPy), to_size_t(len)));
  py_xdecref(pngStrPy);
  out = bmp;
  return true;
}

static std::istream& operator>>(std::istream& s, Point& pt){
  return s >> pt.x >> pt.y;
}

static std::istream& operator>>(std::istream& s, Radii& r){
  return s >> r.x >> r.y;
}

std::vector<PathPt> parse_svg_path(const char* s){
  std::stringstream ss(s);

  char controlChar;
  Point current;
  std::vector<PathPt> points;
  bool prevQuadratic = false;
  bool prevCubic = false;
  Point prev;

  while (ss >> controlChar){
    if (controlChar == 'M'){ // Move to absolute
      prevCubic = prevQuadratic = false;
      Point pt;
      if (ss >> pt.x >> pt.y){
        current = pt;
        points.push_back(PathPt::MoveTo(pt));
        // Absolute line-to coordinates may follow
        while (ss >> pt.x >> pt.y){
          current = pt;
          points.push_back(PathPt::LineTo(pt));
        }
      }
      ss.clear();
    }
    else if (controlChar == 'm'){ // Move to relative
      prevCubic = prevQuadratic = false;
      Point pt;
      if (ss >> pt){
        current += pt;
        points.push_back(PathPt::MoveTo(current));

        while (ss >> pt){
          current += pt;
          points.push_back(PathPt::LineTo(current));
        }
      }
      ss.clear();
    }
    else if (controlChar == 'L'){ // Line-to absolute
      prevCubic = prevQuadratic = false;
      Point pt;
      while (ss >> pt){
        current = pt;
        points.push_back(PathPt::LineTo(pt));
      }
      ss.clear();
    }
    else if (controlChar == 'l'){ // Line-to relative
      prevCubic = prevQuadratic = false;
      Point pt;
      while(ss >> pt){
        current += pt;
        points.push_back(PathPt::LineTo(current));
      }
      ss.clear();
    }
    else if (controlChar == 'V'){ // Vertical line-to absolute
      prevCubic = prevQuadratic = false;
      coord y;
      while (ss >> y){
        current.y = y;
        points.push_back(PathPt::LineTo(current));
      }
      ss.clear();
    }
    else if (controlChar == 'v'){ // Vertical line-to relative
      prevCubic = prevQuadratic = false;
      coord dy;
      while (ss >> dy){
        current.y += dy;
        points.push_back(PathPt::LineTo(current));
      }
      ss.clear();
    }
    else if (controlChar == 'H'){ // Horizontal line to absolute
      prevCubic = prevQuadratic = false;
      coord x;
      while (ss >> x){
        current.x = x;
        points.push_back(PathPt::LineTo(current));
      }
      ss.clear();
    }
    else if (controlChar == 'h'){ // Horizontal line to relative
      prevCubic = prevQuadratic = false;
      coord dx;
      while (ss >> dx){
        current.x += dx;
        points.push_back(PathPt::LineTo(current));
      }
      ss.clear();
    }
    else if (controlChar == 'C'){ // Absolute cubic bezier
      prevQuadratic = false;
      Point p0, p1, p2;
      while (ss >> p0 >> p1 >> p2){
        current = p2;
        points.push_back(PathPt::CubicBezierTo(p2, p0, p1));
      }
      prevCubic = true;
      prev = p1;
      ss.clear();
    }
    else if (controlChar == 'c'){ // Relative cubic bezier
      prevQuadratic = false;
      Point p0, p1, p2;
      while (ss >> p0 >> p1 >> p2){
        p0 += current;
        p1 += current;
        p2 += current;
        points.push_back(PathPt::CubicBezierTo(p2, p0, p1));
        current = p2;
      }
      prevCubic = true;
      prev = p1;
      ss.clear();
    }
    else if (controlChar == 'S'){ // Absolute short hand cubic
      prevQuadratic = false;
      Point p1, p2;
      while (ss >> p1 >> p2){
        Point p0 = prevCubic ? 2 * current - prev : current;

        points.push_back(PathPt::CubicBezierTo(p2, p0, p1));
        current = p2;

        prevCubic = true;
        prev = p1;
      }
      ss.clear();
    }
    else if (controlChar == 's'){ // Relative short hand cubic
      prevQuadratic = false;
      Point p1, p2;
      while (ss >> p1 >> p2){
        Point p0 = prevCubic ? 2 * current - prev : current;
        p1 += current;
        p2 += current;
        points.push_back(PathPt::CubicBezierTo(p2, p0, p1));

        current = p2;
        prevCubic = true;
        prev = p1;
      }
      ss.clear();
    }
    else if (controlChar == 'Q'){ // Absolute quadratic bezier
      prevCubic = false;
      Point p0, p1;
      while (ss >> p0 >> p1){
        // Convert to cubic bezier
        points.push_back(PathPt::CubicBezierTo(p1,
            current + 2.0/3.0 * (p0 - current),
          p1 + 2.0/3.0 * (p0 - p1)));
        current = p1;
      }
      prevQuadratic = true;
      prev = p0;
      ss.clear();
    }
    else if (controlChar == 'q'){ // Relative quadratic bezier
      prevCubic = false;
      Point p0, p1;
      while (ss >> p0 >> p1){
        // Convert to cubic bezier
        p0 += current;
        p1 += current;

        points.push_back(PathPt::CubicBezierTo(p1,
          current + 2.0/3.0 * (p0 - current),
          p1 + 2.0/3.0 * (p0 - p1)));
        current = p1;
      }
      prevQuadratic = true;
      prev = p0;
      ss.clear();
    }
    else if (controlChar == 'T'){ // Absolute short hand quadratic
      prevCubic = false;
      Point p1;
      while (ss >> p1){
        // Convert to cubic bezier
        Point p0 = prevQuadratic ? 2 * current - prev : current;

        points.push_back(PathPt::CubicBezierTo(p1,
            current + 2.0/3.0*(p0 - current),
            p1 + 2.0/3.0*(p0 - p1)));

        current = p1;
        prevQuadratic = true;
        prev = p0;
      }
      ss.clear();
    }
    else if (controlChar == 't'){ // Relative short hand quadratic
      prevCubic = false;
      Point p1;
      while (ss >> p1){
        Point p0 = prevQuadratic ? 2 * current - prev : current;
        p1 += current;
        points.push_back(PathPt::CubicBezierTo(p1,
            current + 2.0 / 3.0 * (p0 - current),
            p1 + 2.0/3.0 * (p0 - p1)));

        current = p1;
        prevQuadratic = true;
        prev = p0;
      }
      ss.clear();
    }
    else if (controlChar == 'z' || controlChar == 'Z'){ // Close path
      prevQuadratic = prevCubic = false;
      points.push_back(PathPt::PathCloser());
    }
    else if (controlChar == 'A'){ // Absolute Arc to
      prevQuadratic = prevCubic = false;
      coord x_axis_rotation;
      Radii r;
      Point pt;
      int large_arc_flag, sweep_flag;
      while (ss >> r >>  x_axis_rotation >>  large_arc_flag >>  sweep_flag >>  pt){
        points.push_back(PathPt::Arc(r, x_axis_rotation, large_arc_flag, sweep_flag, pt));
        current = pt;
      }
      ss.clear();
    }
    else if (controlChar == 'a'){ // Relative arc to
      prevQuadratic = prevCubic = false;
      coord x_axis_rotation;
      Point pt;
      Radii r;
      int large_arc_flag, sweep_flag;
      while (ss >> r >>  x_axis_rotation >>  large_arc_flag >>  sweep_flag >>  pt){
        points.push_back(PathPt::Arc(r, x_axis_rotation, large_arc_flag, sweep_flag, current + pt));
        current += pt;
      }
      ss.clear();
    }
  }
  return points;
}

bool parse_radii(PyObject* args, Radii& out){
  double rx, ry;
  if (!PyArg_ParseTuple(args, "dd", &rx, &ry)){
    return false;
  }
  out.x = rx;
  out.y = ry;
  return true;
}

bool parse_rgb_color(PyObject* args, ColRGB& color){
  Color rgba;
  if (!parse_color(args, rgba, false)){
    return false;
  }
  color = strip_alpha(rgba);
  return true;
}

bool parse_tri(PyObject* args, Tri* out){
  coord x0, y0, x1, y1, x2, y2;
  if (PyArg_ParseTuple(args, "dddddd", &x0, &y0, &x1, &y1, &x2, &y2)){
    *out = Tri(Point(x0, y0), Point(x1, y1), Point(x2, y2));
    return true;
  }
  return false;
}

utf8_string points_to_svg_path_string(const std::vector<PathPt>& points){
  std::stringstream ss;
  for (size_t i = 0; i != points.size(); i++){
    const PathPt& pt = points[i];
    if (pt.IsArc()){
      ss << "A" << " " <<
      pt.r.x << " " <<
      pt.r.y << " " <<
      pt.axis_rotation << " " <<
      pt.large_arc_flag << " " <<
      pt.sweep_flag << " " <<
      pt.p.x << " " <<
      pt.p.y << " ";
    }
    else if (pt.ClosesPath()){
      ss << "z ";
    }
    else if (pt.IsCubicBezier()){
      ss << "C " << pt.c.x << "," << pt.c.y << " " << pt.d.x << "," << pt.d.y << " " << pt.p.x << "," << pt.p.y << " ";
    }
    else if (pt.IsLine()){
      ss << "L " << pt.p.x << "," << pt.p.y << " ";
    }
    else if (pt.IsMove()){
      ss << "M " << pt.p.x << "," << pt.p.y << " ";
    }
    else {
      assert(false); // Invalid PathPt type
    }
  }
  std::string pathStr = ss.str();
  return utf8_string(pathStr.substr(0, pathStr.size() - 1));
}

PyObject* python_bool(bool v){
  if (v){
    Py_RETURN_TRUE;
  }
  Py_RETURN_FALSE;
}

utf8_string spline_to_svg_path_string(const std::vector<Point>& points){
  std::stringstream ss;
  const Point& p0 = points[0];
  coord x1 = p0.x;
  coord y1 = p0.y;

  const Point& p1 = points[1];
  coord c = p1.x;
  coord d = p1.y;
  coord x3 = (x1 + c) / 2;
  coord y3 = (y1 + d) / 2;

  ss << "M " << x1 << "," << y1 << " ";
  ss << "L " << x3 << "," << y3 << " ";

  for (size_t i = 2; i < points.size(); i++){
    const Point& pt = points[i];
    x1 = x3;
    y1 = y3;
    coord x2 = c;
    coord y2 = d;
    c = pt.x;
    d = pt.y;
    x3 = (x2 + c) / 2;
    y3 = (y2 + d) / 2;
    ss << "C " <<
      x1 << "," << y1 << " " <<
      x2 << "," << y2 << " " <<
      x3 << "," << y3 << " ";
  }

  ss << "L " << c << "," << d;
  return utf8_string(ss.str());
}

PyObject* to_py_png_string(const Bitmap& bmp){
  std::string pngString(to_png_string(bmp));
  return PyBytes_FromStringAndSize(pngString.c_str(), resigned(pngString.size()));
}

PyObject* to_py_raw_rgb_string(const Bitmap& bmp){
  std::string str;
  str.reserve(to_size_t(area(bmp.GetSize())*3));
  for (int y = 0; y != bmp.m_h; y++){
    for (int x = 0; x != bmp.m_w; x++){
      Color c(get_color_raw(bmp, x, y));
      str += static_cast<char>(c.r);
      str += static_cast<char>(c.g);
      str += static_cast<char>(c.b);
    }
  }

  return PyBytes_FromStringAndSize(str.c_str(), resigned(str.size()));
}

static utf8_string get_repr(const Color& c){
  return bracketed(str_rgba(c));
}

bool coord_from_py_noerr(PyObject* obj, coord& c){
  if (!PyNumber_Check(obj)){
    return false;
  }
  PyObject* pythonFloat = PyNumber_Float(obj);
  c = PyFloat_AsDouble(pythonFloat);
  return true;
}

bool point_from_sequence_noerr(PyObject* obj, Point& p){
  if (!PySequence_Check(obj)){
    return false;
  }
  if (PySequence_Length(obj) != 2){
    return false;
  }
  ScopedRef py_x(PySequence_GetItem(obj, 0));
  coord x = 0.0;
  if (!coord_from_py_noerr(*py_x, x)){
    return false;
  }
  ScopedRef py_y(PySequence_GetItem(obj, 1));
  coord y = 0.0;
  if (!coord_from_py_noerr(*py_y, y)){
    return false;
  }
  p.x = x;
  p.y = y;
  return true;
}

static utf8_string get_repr(const Point& pt){
  std::stringstream ss;
  ss << "(" << pt.x << ", " << pt.y << ")";
  return utf8_string(ss.str());
}

static utf8_string get_repr(const ColorStop& stop){
  std::stringstream ss;
  ss << stop.GetOffset() << ", " << get_repr(stop.GetColor());
  return bracketed(utf8_string(ss.str()));
}

static utf8_string get_repr(const color_stops_t& stops){
  std::stringstream ss;
  ss << "[";
  for (color_stops_t::const_iterator it = begin(stops); it != end(stops); ++it){
    ss << get_repr(*it);
    if (it != stops.end() - 1){
      ss << ",";
    }
  }
  ss << "]";
  return utf8_string(ss.str());
}

utf8_string get_repr(const LinearGradient& g){
  std::stringstream ss;
  ss << "LinearGradient(" << g.GetAngle().Rad() << ", " << get_repr(g.GetStops()) << ")";
  return utf8_string(ss.str());
}

utf8_string get_repr(const RadialGradient& g){
  std::stringstream ss;
  ss << "RadialGradient(" << get_repr(g.GetCenter()) << ", " << get_repr(g.GetStops()) << ")";
  return utf8_string(ss.str());
}

PyObject* unpack_once(PyObject* obj){
  // Fixme: What's this?
  const int n = PySequence_Length(obj);
  if (n == 1){
    PyObject* item = PySequence_GetItem(obj, 0);
    if (PySequence_Check(item) && !PyUnicode_Check(item)){
      return item;
    }
  }
  return obj;
}

bool is_string_sequence(PyObject* obj){
  if (!PySequence_Check(obj) || PyUnicode_Check(obj)){
    return false;
  }

  obj = unpack_once(obj);
  const int n = PySequence_Length(obj);
  if (n == 0){
    return false;
  }
  for (int i = 0; i != n; i++){
    PyObject* item = PySequence_GetItem(obj,i);
    if (!PyUnicode_Check(item)){
      return false;
    }
  }
  return true;
}

std::vector<utf8_string> parse_string_sequence(PyObject* obj){
  obj = unpack_once(obj);
  assert(!PyUnicode_Check(obj));
  std::vector<utf8_string> strings;
  const int n = PySequence_Length(obj);
  for (int i = 0; i != n; i++){
    ScopedRef item(PySequence_GetItem(obj, i));
    auto maybeString = parse_py_unicode(*item);
    if (maybeString.IsSet()){
      strings.push_back(maybeString.Get());
    }
  }
  return strings;
}

bool py_error_occurred(){
  // PyErr_Occurred returns a borrowed reference
  // to the exception type (or nullptr if none).
  return PyErr_Occurred() != nullptr;
}

bool py_load_error_occurred(){
  PyObject* errType = PyErr_Occurred();
  if (errType == nullptr){
    return false;
  }
  PyObject* loadError = get_load_exception_type();
  int result = PyErr_GivenExceptionMatches(errType, loadError);
  return result != 0;
}

bool py_save_error_occurred(){
  PyObject* errType = PyErr_Occurred();
  if (errType == nullptr){
    return false;
  }
  PyObject* saveError = get_save_exception_type();
  int result = PyErr_GivenExceptionMatches(errType, saveError);
  return result != 0;
}


utf8_string py_error_string(){
  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceBack = nullptr;

  PyErr_Fetch(&type, &value, &traceBack);
  if (value == nullptr){
    return utf8_string("");
  }
  PyObject* exceptionStr = PyObject_Str(value);
  py_xdecref(type);
  py_xdecref(value);
  py_xdecref(traceBack);
  if (exceptionStr == nullptr){
    return utf8_string("");
  }
  PyObject* bytes = PyUnicode_AsUTF8String(exceptionStr);
  char* str(PyBytes_AsString(bytes)); // Fixme
  utf8_string errorString(str);
  py_xdecref(exceptionStr);
  py_xdecref(bytes);
  return utf8_string(errorString);
}

std::vector<utf8_string> parse_traceback(PyObject* tb){

  PyObject* tracebackModule = PyImport_ImportModule("traceback");
  assert(tracebackModule != nullptr);

  PyObject* dict = PyModule_GetDict(tracebackModule);
  PyObject* format_tb = PyDict_GetItemString(dict, "format_tb");

  PyObject* args = Py_BuildValue("Oi", tb, 10);
  PyObject* tbList = PyEval_CallObject(format_tb, args);
  assert(PySequence_Check(tbList));
  int n = PySequence_Length(tbList);
  std::vector<utf8_string> v;
  v.reserve(to_size_t(n));
  for (int i = 0; i != n; i++){
    PyObject* item = PySequence_GetItem(tbList, i);
    v.push_back(object_string(item));
    py_xdecref(item);
  }
  py_xdecref(tracebackModule);
  return v;
}

static utf8_string get_name(PyObject* o){
  ScopedRef nameStr(PyObject_GetAttrString(o, "__name__"));
  if (*nameStr == nullptr){
    return "";
  }
  else{
    return parse_py_unicode(*nameStr).Or("");
  }
}

static utf8_string decode_or_die(PyObject* obj){
  PyObject* pyBytes = PyUnicode_AsUTF8String(obj);
  assert(pyBytes != nullptr);
  char* utf8 = PyBytes_AsString(pyBytes);
  assert(utf8 != nullptr);
  utf8_string str(utf8);
  py_xdecref(pyBytes);
  return utf8;
}

static void decode_syntax_error_tuple(PyObject* tuple, FaintPyExc& info){
  int numItems = PySequence_Length(tuple);
  if (numItems == 0){
    return;
  }
  ScopedRef message(PySequence_GetItem(tuple, 0));
  if (*message != nullptr){
    if (PyUnicode_Check(*message)){
      info.message = decode_or_die(*message);
    }
  }
  if (numItems == 1){
    return;
  }
  ScopedRef syntaxErrTuple(PySequence_GetItem(tuple,1));
  if (!PyTuple_Check(*syntaxErrTuple)){
    return;
  }

  int numExtra = PySequence_Length(*syntaxErrTuple);
  if (numExtra == 0){
    return;
  }
  FaintPySyntaxError syntaxError;

  ScopedRef fileName(PySequence_GetItem(*syntaxErrTuple, 0));
  if (*fileName != nullptr && PyUnicode_Check(*fileName)){
    syntaxError.file = decode_or_die(*fileName);
  }
  if (numExtra > 1){
    ScopedRef line(PySequence_GetItem(*syntaxErrTuple, 1));
    if (*line != nullptr && PyLong_Check(*line)){
      syntaxError.line = PyLong_AsLong(*line);
    }
    if (numExtra > 2){
      ScopedRef column(PySequence_GetItem(*syntaxErrTuple, 2));
      if (*column != nullptr && PyLong_Check(*column)){
        syntaxError.col = PyLong_AsLong(*column);
      }

      if (numExtra > 3){
        ScopedRef code(PySequence_GetItem(*syntaxErrTuple, 3));
        if (*code != nullptr && PyUnicode_Check(*code)){
          syntaxError.code = decode_or_die(*code);
        }
      }
    }
  }
  info.syntaxErrorInfo.Set(syntaxError);
}

void decode_syntax_error_object(PyObject* error, FaintPyExc& info){
  ScopedRef msg(PyObject_GetAttrString(error, "msg"));
  ScopedRef lineNo(PyObject_GetAttrString(error, "lineno"));
  ScopedRef offset(PyObject_GetAttrString(error, "offset"));
  ScopedRef text(PyObject_GetAttrString(error, "text"));
  ScopedRef filename(PyObject_GetAttrString(error, "filename"));

  FaintPySyntaxError errInfo;
  if (*msg != nullptr && PyUnicode_Check(*msg)){
    info.message = decode_or_die(*msg);
  }
  if (*lineNo != nullptr && PyLong_Check(*lineNo)){
    errInfo.line = PyLong_AsLong(*lineNo);
  }
  if (*offset != nullptr && PyLong_Check(*offset)){
    errInfo.col = PyLong_AsLong(*offset);
  }
  if (*text != nullptr && PyUnicode_Check(*text)){
    errInfo.code = decode_or_die(*text);
  }
  if (*filename != nullptr && PyUnicode_Check(*filename)){
    errInfo.file = decode_or_die(*filename);
  }
  info.syntaxErrorInfo.Set(errInfo);
}

FaintPyExc py_error_info(){
  PyObject* type = nullptr;
  PyObject* value = nullptr;
  PyObject* traceback = nullptr;
  PyErr_Fetch(&type, &value, &traceback);

  FaintPyExc info;
  if (type != nullptr){
    info.type = get_name(type);
  }
  if (value != nullptr){
    if (PyUnicode_Check(value)){
      info.message = decode_or_die(value);
    }
    if ((PySequence_Check(value) && info.type == "SyntaxError") ||
      info.type == "IndentationError"){

      // When the Syntax error is in executed text, it's a tuple
      // rather than an exception object.
      decode_syntax_error_tuple(value, info);
    }
    else if (info.type == "SyntaxError"){
      decode_syntax_error_object(value, info);
    }
    else{
      info.message = object_string(value);
    }
  }
  if (traceback != nullptr){
    info.stackTrace = parse_traceback(traceback);
  }
  return info;
}

PyObject* get_load_exception_type(){
  WritableStr typeString("ifaint.LoadError");
  static PyObject* faintPyLoadError = PyErr_NewException(typeString.c_str(),
    nullptr, nullptr);
  return faintPyLoadError;
}

PyObject* get_save_exception_type(){
  WritableStr typeString("ifaint.SaveError");
  static PyObject* faintPySaveError = PyErr_NewException(typeString.c_str(),
    nullptr, nullptr);
  return faintPySaveError;
}

void py_set_value_error(const utf8_string& str){
  ScopedRef obj(build_unicode(str));
  PyErr_SetObject(PyExc_ValueError, *obj);
}

PyObject* set_error(const PythonError& err){
  err.SetError();
  return nullptr;
}

} // namespace
