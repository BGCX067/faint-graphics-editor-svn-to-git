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
#include "py_bitmap.hh"
#include "py_canvas.hh"
#include "py_something.hh"
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
PyObject* build_color_tuple( const faint::Color& );
PyObject* build_coord( faint::coord );
PyObject* build_intsize( const IntSize& );
PyObject* build_point( const Point& );
PyObject* build_radian( faint::radian );
PyObject* build_rect( const Rect& );
bool invalid_color( int r, int g, int b, int a=255);
bool invalid_pixel_pos( int x, int y, const faint::Bitmap&);
bool invalid_pixel_pos( const IntPoint&, const faint::Bitmap&);
bool objects_from_args( PyObject* args, objects_t& out );
bool parse_color( PyObject* sequence, faint::Color&, bool allowAlpha=true );
bool parse_coord( PyObject* args, faint::coord* out );
bool parse_point( PyObject* args, Point* out );
bool parse_point2( PyObject* args, Point* out1, Point* out2 );
bool parse_intpoint( PyObject* args, IntPoint* out );
bool parse_intrect( PyObject* args, IntRect* out );
bool parse_objects( PyObject* args, objects_t* out );
std::vector<PathPt> parse_points( const char* s );
bool parse_radian( PyObject* args, faint::radian* out );
bool parse_rect( PyObject* args, Rect* out );
bool parse_rgb_color( PyObject* sequence, faint::Color& );
bool parse_tri( PyObject* args, Tri* out );
faint::PyPoint point_to_py( const Point& );
std::string points_to_svg_path_string( const std::vector<PathPt>& );
PyObject* python_bool( bool );
std::string spline_to_svg_path_string( const std::vector<Point>& );

namespace faint{
  // Returns true if the object is a sequence of strings, or
  // a single nested sequence with strings.
  bool is_string_sequence( PyObject* );
  std::vector<std::string> parse_string_sequence( PyObject* );

  bool py_error_occurred();
  std::string py_error_string();
}

#endif
