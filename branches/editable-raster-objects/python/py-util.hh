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
#include "util/commonfwd.hh"
#include "util/imagelist.hh" // For index_t
#include "py-bitmap.hh"
#include "py-canvas.hh"
#include "py-something.hh"
class ObjRaster;
class ObjText;
class ObjSpline;
class ObjPath;
class ObjEllipse;

namespace faint {
  struct PyPoint{
    PyObject* x;
    PyObject* y;
  };
}

faint::Bitmap* as_Bitmap( PyObject* );
CanvasInterface* as_Canvas( PyObject* );
ObjEllipse* as_ObjEllipse( PyObject* );
ObjPath* as_ObjPath( PyObject* );
std::pair<CanvasInterface*, ObjRaster*> as_ObjRaster( PyObject* );
ObjSpline* as_ObjSpline( PyObject* );
std::pair<CanvasInterface*, ObjText*> as_ObjText( PyObject* );
smthObject* as_smthObject( PyObject* );
PyObject* build_angle_span( const AngleSpan& );
PyObject* build_bitmap( const faint::Bitmap& );
PyObject* build_color_tuple( const faint::Color& );
PyObject* build_color_stop( const faint::ColorStop& );
PyObject* build_color_stops( const faint::LinearGradient& );
PyObject* build_color_stops( const faint::RadialGradient& );
PyObject* build_coord( faint::coord );
PyObject* build_draw_source( const faint::DrawSource& );
PyObject* build_frame(CanvasInterface*, const index_t&);
PyObject* build_gradient( const faint::Gradient& );
PyObject* build_object_list( const objects_t&, CanvasInterface*, const FrameId& );
PyObject* build_pattern( const faint::Pattern& );
PyObject* build_intpoint( const IntPoint& );
PyObject* build_intsize( const IntSize& );
PyObject* build_point( const Point& );
PyObject* build_radian( faint::radian );
PyObject* build_radii( const Radii& );
PyObject* build_rect( const Rect& );
bool invalid_color( int r, int g, int b, int a=255);
bool invalid_pixel_pos( int x, int y, const faint::Bitmap&);
bool invalid_pixel_pos( const IntPoint&, const faint::Bitmap&);
bool objects_from_args( PyObject* args, objects_t& out );
std::string object_string(PyObject*);
bool parse_angle_span( PyObject* sequence, AngleSpan& out );
bool parse_bool( PyObject* sequence, bool& out );
bool parse_color( PyObject* sequence, faint::Color&, bool allowAlpha=true );
bool parse_color_stop( PyObject*, faint::ColorStop& );
bool parse_coord( PyObject* args, faint::coord* out );
bool parse_draw_source(PyObject* args, faint::DrawSource& out);
bool parse_point( PyObject* args, Point* out );
bool parse_point2( PyObject* args, Point* out1, Point* out2 );
bool parse_intpoint( PyObject* args, IntPoint* out );
bool parse_intsize( PyObject* args, IntSize* out );
bool parse_intrect( PyObject* args, IntRect* out );
bool parse_objects( PyObject* args, objects_t* out );
bool parse_png_bitmap( PyObject* args, faint::Bitmap& out );
std::vector<PathPt> parse_points( const char* s );
bool parse_radian( PyObject* args, faint::radian* out );
bool parse_radii( PyObject* args, Radii& out );
bool parse_rect( PyObject* args, Rect* out );
bool parse_rgb_color( PyObject* sequence, faint::Color& );
bool parse_tri( PyObject* args, Tri* out );
faint::PyPoint point_to_py( const Point& );
std::string points_to_svg_path_string( const std::vector<PathPt>& );
PyObject* python_bool( bool );
std::string spline_to_svg_path_string( const std::vector<Point>& );
PyObject* to_py_png_string( const faint::Bitmap& );
PyObject* to_py_raw_rgb_string( const faint::Bitmap& );
std::string get_repr( const faint::LinearGradient& );
std::string get_repr( const faint::RadialGradient& );

bool point_from_sequence_noerr( PyObject*, Point& );

struct FaintPyExc{
  std::string type;
  std::string value;
  std::vector<std::string> stackTrace;
};


namespace faint{
  PyObject* get_save_exception_type();
  PyObject* get_load_exception_type();

  // Returns true if the object is a sequence of strings, or
  // a single nested sequence with strings.
  bool is_string_sequence( PyObject* );
  std::vector<std::string> parse_string_sequence( PyObject* );

  bool py_error_occurred();
  bool py_load_error_occurred();
  bool py_save_error_occurred();
  std::string py_error_string();
  std::string stack_trace_str( PyObject* traceback );
  FaintPyExc py_error_info();

  bool PyRadian_Check(PyObject*);
  radian PyRadian_AsRadian(PyObject*);
  bool PyPoint_Check(PyObject*);
}

#endif
