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

#ifndef FAINT_PY_UTIL_HH
#define FAINT_PY_UTIL_HH
#include <string>
#include <utility> // For std::pair
#include <vector>
#include "python/py-bitmap.hh"
#include "python/py-canvas.hh"
#include "python/py-something.hh"
#include "util/common-fwd.hh"
#include "util/distinct.hh"
#include "util/index.hh"
#include "util/optional.hh"

namespace faint{
class ObjRaster;
class ObjText;
class ObjSpline;
class ObjPath;
class ObjEllipse;

class category_py_util;
typedef Distinct<bool, category_py_util, 0> allow_empty;

// Attempts to decode the PyUnicode-object into an utf8-string.
// Sets a Python error and returns nothing on failure.
Optional<utf8_string> parse_py_unicode(PyObject*);
Bitmap* as_Bitmap(PyObject*);
Optional<Gradient> as_Gradient(PyObject*);
Pattern* as_Pattern(PyObject*);
Object* as_Object(PyObject*);
std::pair<Canvas*, ObjRaster*> as_ObjRaster(PyObject*);
std::pair<Canvas*, ObjText*> as_ObjText(PyObject*);
smthObject* as_smthObject(PyObject*);
PyObject* build_bitmap(const Bitmap&);
PyObject* build_color_tuple(const Color&);
PyObject* build_paint(const Paint&);
PyObject* build_frame(Canvas*, const index_t&);
PyObject* build_object_list(const objects_t&, Canvas*, const FrameId&);
PyObject* build_radii(const Radii&);
PyObject* build_unicode(const utf8_string&);
bool invalid_color(int r, int g, int b, int a=255);
bool invalid_pixel_pos(int x, int y, const Bitmap&);
bool invalid_pixel_pos(const IntPoint&, const Bitmap&);

// Attempts to parse objects from args, which can either be a sequence
// of objects or contain a single sequence with objects.  Empty lists
// are reported as an error, except for a single nested list when
// allow_empty is true, to allow:
// > some_py_func(objects) # objects can be []
// but not
// > some_py_func()
bool objects_from_args(PyObject* args, const allow_empty&, objects_t& out);
utf8_string object_string(PyObject*);
bool parse_color(PyObject* sequence, Color&, bool allowAlpha=true);
bool parse_color_stop(PyObject*, ColorStop&);
bool parse_paint(PyObject* args, Paint& out);
bool parse_intpoint(PyObject* args, IntPoint* out);
bool parse_objects(PyObject* args, objects_t* out);
bool parse_png_bitmap(PyObject* args, Bitmap& out);
std::vector<PathPt> parse_svg_path(const char*);
bool parse_radii(PyObject* args, Radii& out);
bool parse_rgb_color(PyObject* sequence, ColRGB&);
bool parse_tri(PyObject* args, Tri* out);
utf8_string points_to_svg_path_string(const std::vector<PathPt>&);
PyObject* python_bool(bool);
utf8_string spline_to_svg_path_string(const std::vector<Point>&);
PyObject* to_py_png_string(const Bitmap&);
PyObject* to_py_raw_rgb_string(const Bitmap&);
utf8_string get_repr(const LinearGradient&);
utf8_string get_repr(const RadialGradient&);
bool point_from_sequence_noerr(PyObject*, Point&);

class FaintPySyntaxError{
public:
  FaintPySyntaxError()
    : line(0), col(0)
  {}
  utf8_string file;
  int line;
  int col;
  utf8_string code;
};

class FaintPyException{
  utf8_string type;
  utf8_string message;
};

class FaintPyExc{
public:
  utf8_string type;
  utf8_string message;
  std::vector<utf8_string> stackTrace;
  Optional<FaintPySyntaxError> syntaxErrorInfo;
};

PyObject* get_save_exception_type();
PyObject* get_load_exception_type();

// Returns true if the object is a sequence of strings, or
// a single nested sequence with strings.
bool is_string_sequence(PyObject*);
std::vector<utf8_string> parse_string_sequence(PyObject*);

bool py_error_occurred();
bool py_load_error_occurred();
bool py_save_error_occurred();
void py_set_value_error(const utf8_string&);
utf8_string py_error_string();

utf8_string stack_trace_str(PyObject* traceback);
FaintPyExc py_error_info();

} // namespace

#endif
