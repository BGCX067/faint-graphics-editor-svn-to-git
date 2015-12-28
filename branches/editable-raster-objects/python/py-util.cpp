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
#include "python/pythoninclude.hh"
#include "python/py-frame.hh"
#include "python/py-linear-gradient.hh"
#include "python/py-pattern.hh"
#include "python/py-radial-gradient.hh"
#include "python/py-util.hh"
#include "python/writable-str.hh"
#include "objects/objellipse.hh"
#include "objects/objpath.hh"
#include "objects/objraster.hh"
#include "objects/objspline.hh"
#include "objects/objtext.hh"
#include "util/canvasinterface.hh"
#include "util/formatting.hh"
#include "util/util.hh" // For to_png_string

typedef std::is_same<faint::coord, double> ensure_coord_double;

faint::Bitmap* as_Bitmap( PyObject* obj ){
  if ( !PyObject_IsInstance( obj, (PyObject*)&BitmapType ) ){
    PyErr_SetString(PyExc_ValueError, "Not a Bitmap" );
    return nullptr;
  }
  bitmapObject* py_bitmap = (bitmapObject*)obj;
  return py_bitmap->bmp;
}


CanvasInterface* as_Canvas( PyObject* obj ){
  if ( !PyObject_IsInstance( obj, (PyObject*)&CanvasType ) ){
    PyErr_SetString(PyExc_ValueError, "Not a Canvas" );
    return nullptr;
  }
  canvasObject* py_canvas = (canvasObject*)obj;
  return py_canvas->canvas;
}

static faint::Gradient* as_Gradient( PyObject* obj ){
  if ( PyObject_IsInstance(obj, (PyObject*)&LinearGradientType) ){
    linearGradientObject* pyGradient = (linearGradientObject*)(obj);
    return new faint::Gradient(*pyGradient->gradient);
  }
  else if ( PyObject_IsInstance(obj, (PyObject*)&RadialGradientType) ){
    radialGradientObject* pyGradient = (radialGradientObject*)(obj);
    return new faint::Gradient(*pyGradient->gradient);
  }

  PyErr_SetString(PyExc_TypeError, "The argument must be a LinearGradient or a RadialGradient object");
  return nullptr;
}

ObjEllipse* as_ObjEllipse( PyObject* obj ){
  smthObject* smthObj = as_smthObject( obj );
  if ( smthObj == nullptr ){
    return nullptr;
  }
  ObjEllipse* ellipse = dynamic_cast<ObjEllipse*>( smthObj->obj );
  return ellipse;
}

ObjPath* as_ObjPath( PyObject* obj ){
  smthObject* smthObj = as_smthObject( obj );
  if ( smthObj == nullptr ){
    return nullptr;
  }
  ObjPath* path = dynamic_cast<ObjPath*>( smthObj->obj );
  return path;
}

std::pair<CanvasInterface*, ObjRaster*> as_ObjRaster( PyObject* obj ){
  smthObject* smth = as_smthObject( obj );
  if ( smth == nullptr ){
    return std::make_pair<CanvasInterface*, ObjRaster*>(0,0);
  }
  ObjRaster* raster = dynamic_cast<ObjRaster*> ( smth->obj );
  if ( raster == nullptr ){
    PyErr_SetString( PyExc_ValueError, "Not a Raster object" );
    return std::make_pair<CanvasInterface*, ObjRaster*>(0,0);
  }
  return std::make_pair(smth->canvas, raster);
}

ObjSpline* as_ObjSpline( PyObject* obj ){
  smthObject* smth = as_smthObject( obj );
  if ( smth == nullptr ){
    return nullptr;
  }
  ObjSpline* spline = dynamic_cast<ObjSpline*>( smth->obj );
  return spline;
}

std::pair<CanvasInterface*, ObjText*> as_ObjText( PyObject* obj ){
  smthObject* smth = as_smthObject( obj );
  if ( smth == nullptr ){
    return std::make_pair<CanvasInterface*, ObjText*>((CanvasInterface*)0,(ObjText*)0);
  }
  ObjText* text = dynamic_cast<ObjText*> ( smth->obj );
  if ( text == nullptr ){
    PyErr_SetString( PyExc_ValueError, "Not a Text object" );
    return std::make_pair<CanvasInterface*, ObjText*>(0,0);
  }
  return std::make_pair(smth->canvas, text);
}

faint::Pattern* as_Pattern( PyObject* obj ){
  if ( !PyObject_IsInstance(obj, (PyObject*)&PatternType) ){
    PyErr_SetString(PyExc_TypeError, "The argument must be a Pattern object");
    return nullptr;
  }
  patternObject* pyPattern = (patternObject*)(obj);
  return pyPattern->pattern;
}


smthObject* as_smthObject( PyObject* obj ){
  if ( !PyObject_IsInstance( obj, (PyObject*)&SmthType ) ){
    PyErr_SetString(PyExc_TypeError, "Not a Faint Object" );
    return nullptr;
  }
  return (smthObject*)obj;
}

PyObject* build_angle_span( const AngleSpan& span ){
  return Py_BuildValue("dd", span.start, span.stop);
}

PyObject* build_bitmap( const faint::Bitmap& bmp ){
  assert(bitmap_ok(bmp));
  bitmapObject* py_bitmap = (bitmapObject*)BitmapType.tp_alloc(&BitmapType, 0);
  py_bitmap->bmp = new faint::Bitmap(bmp); // Fixme, handle exception
  return (PyObject*)py_bitmap;
}

PyObject* build_color_tuple( const faint::Color& color ){
  return Py_BuildValue("iiii", color.r, color.g, color.b, color.a );
}

PyObject* build_color_stop( const faint::ColorStop& stop ){
  faint::Color c(stop.GetColor());
  double offset(stop.GetOffset());
  return Py_BuildValue("d(iiii)", offset, c.r, c.g, c.b, c.a);
}

template<typename GradientType>
PyObject* build_color_stops_either( const GradientType& gr ){
  const int numStops = gr.GetNumStops();
  PyObject* list = PyList_New( numStops );
  for ( int i = 0; i != numStops; i++ ){
    PyList_SetItem( list, i, build_color_stop(gr.GetStop(i)) );
  }
  return list;
}

PyObject* build_color_stops( const faint::LinearGradient& lg ){
  return build_color_stops_either(lg);
}

PyObject* build_color_stops( const faint::RadialGradient& rg ){
  return build_color_stops_either(rg);
}

PyObject* build_coord( faint::coord value ){
  static_assert(ensure_coord_double::value, "faint::coord must be double for build_coord");
  return Py_BuildValue("d", value );
}

PyObject* build_draw_source( const faint::DrawSource& src ){
  return dispatch(src, build_color_tuple, build_pattern, build_gradient );
}

PyObject* build_frame( CanvasInterface* canvas, const index_t& index ){
  assert( index < canvas->GetNumFrames() );
  frameObject* py_frame = (frameObject*)(FrameType.tp_alloc(&FrameType, 0));
  py_frame->canvas = canvas;
  py_frame->canvasId = canvas->GetId();
  py_frame->frameId = canvas->GetFrame(index).GetId();
  return (PyObject*)py_frame;
}

PyObject* build_gradient( const faint::Gradient& gradient ){
  if ( gradient.IsLinear() ){
    linearGradientObject* py_gradient = (linearGradientObject*)LinearGradientType.tp_alloc(&LinearGradientType, 0);
    py_gradient->gradient = new faint::LinearGradient(gradient.GetLinear());
    return (PyObject*)py_gradient;
  }
  else if ( gradient.IsRadial() ){
    radialGradientObject* py_gradient = (radialGradientObject*)RadialGradientType.tp_alloc(&RadialGradientType, 0);
    py_gradient->gradient = new faint::RadialGradient(gradient.GetRadial());
    return (PyObject*)py_gradient;
  }
  else {
    assert(false);
    return Py_BuildValue("");
  }
}

PyObject* build_object_list( const objects_t& objects, CanvasInterface* canvas, const FrameId& frameId ){
  PyObject* py_list = PyList_New(0);
  for ( Object* obj : objects ){
    PyList_Append( py_list, pythoned( obj, canvas, frameId ) );
  }
  return py_list;
}

PyObject* build_pattern( const faint::Pattern& pattern ){
  patternObject* py_pattern = (patternObject*)PatternType.tp_alloc(&PatternType, 0);
  py_pattern->pattern = new faint::Pattern(pattern);
  return (PyObject*)py_pattern;
}

PyObject* build_intpoint( const IntPoint& p ){
  return Py_BuildValue("ii", p.x, p.y);
}

PyObject* build_intsize( const IntSize& size ){
  return Py_BuildValue("ii", size.w, size.h);
}

PyObject* build_point( const Point& pt ){
  return Py_BuildValue("dd", pt.x, pt.y );
}

PyObject* build_radian( faint::radian value ){
  return Py_BuildValue("d", value);
}

PyObject* build_radii( const Radii& radii ){
  return Py_BuildValue("dd", radii.x, radii.y );
}

PyObject* build_rect( const Rect& r ){
  return Py_BuildValue("dddd", r.x, r.y, r.w, r.h);
}

bool invalid_color( int r, int g, int b, int a){
  if ( !( 0 <= r && r <= 255 ) ){
    PyErr_SetString( PyExc_ValueError, "Invalid rgba-color: Red component out of range (0-255)." );
    return true;
  }
  else if ( !( 0 <= g && g <= 255 ) ){
    PyErr_SetString( PyExc_ValueError, "Invalid rgba-color: Green component out of range (0-255)." );
    return true;
  }
  else if ( !( 0 <= b && b <= 255 ) ){
    PyErr_SetString( PyExc_ValueError, "Invalid rgba-color: Blue component out of range (0-255)." );
    return true;
  }
  else if ( !( 0 <= a && a <= 255 ) ){
    PyErr_SetString( PyExc_ValueError, "Invalid rgba-color: Alpha component out of range (0-255)." );
    return true;
  }

  // Color is valid
  return false;
}

bool invalid_pixel_pos( int x, int y, const faint::Bitmap& bmp){
  return invalid_pixel_pos(IntPoint(x,y), bmp);
}

bool invalid_pixel_pos( const IntPoint& pos, const faint::Bitmap& bmp){
  if ( !point_in_bitmap(bmp, pos) ){
    PyErr_SetString( PyExc_ValueError, "Point outside image" );
    return true;
  }
  return false;
}

bool objects_from_args( PyObject* args, objects_t& v ){
  const int numArg = PySequence_Length( args );
  bool nestedSequence = ( numArg == 1 && PySequence_Check( PySequence_GetItem( args, 0 ) ) );
  PyObject* sequence = nestedSequence ? PySequence_GetItem( args, 0 ) : args;
  const int n = nestedSequence ? PySequence_Length( sequence ) : numArg;

  if ( n == 0 ){
    PyErr_SetString( PyExc_ValueError, "No objects specified");
    return false;
  }

  for ( int i = 0; i != n; i++ ){
    PyObject* item = PySequence_GetItem( sequence, i );
    smthObject* obj = as_smthObject( item );
    if ( obj == nullptr ){
      PyErr_SetString(PyExc_TypeError, "That's not a Faint object.");
      return false;
    }
    v.push_back( obj->obj );
  }
  return true;
}

std::string object_string( PyObject* obj ){
  PyObject* pyStr = PyObject_Str(obj);
  if ( pyStr == nullptr ){
    return "";
  }
  std::string str(PyString_AsString(pyStr));
  faint::py_xdecref(pyStr);
  return str;
}

bool parse_angle_span( PyObject* sequence, AngleSpan& out ){
  faint::radian start, stop;
  if ( !PyArg_ParseTuple(sequence, "dd", &start, &stop) ){
    return false;
  }
  out = AngleSpan(start, stop);
  return true;
}

bool parse_bool( PyObject* args, bool& out ){
  if ( PySequence_Length(args) != 1 ){
    PyErr_SetString(PyExc_ValueError, "Expected True/False");
    return false;
  }
  PyObject* obj = PySequence_GetItem(args, 0);
  out = 1 == PyObject_IsTrue(obj);
  faint::py_xdecref(obj);
  return true;
}

bool parse_color( PyObject* args, faint::Color& color, bool allowAlpha ){
  int r = 255;
  int g = 255;
  int b = 255;
  int a = 255;

  bool ok = PyArg_ParseTuple( args, "iiii", &r, &g, &b, &a ) ||
    PyArg_ParseTuple( args, "iii", &r, &g, &b ) ||
    PyArg_ParseTuple( args, "(iiii)", &r, &g, &b, &a ) ||
    PyArg_ParseTuple( args, "(iii)", &r, &g, &b );

  if ( !ok ){
    PyErr_SetString( PyExc_TypeError, "Invalid color specification. Valid formats for colors are r,g,b and r,g,b,a" );
    return false;
  }
  PyErr_Clear();

  if ( invalid_color( r, g, b, a ) ){
    return false;
  }

  if ( !allowAlpha && a != 255 ){
    PyErr_SetString( PyExc_ValueError, "Alpha value not supported for this function");
    return false;
  }

  color = faint::color_from_ints(r,g,b,a);
  return true;
}

bool parse_color_stop( PyObject* obj, faint::ColorStop& stop ){
  if ( PySequence_Length(obj) != 2 ){
    PyErr_SetString(PyExc_ValueError, "Color stop must be specified using offset,(r,g,b[,a])");
    return false;
  }
  PyObject* pyOffset = PySequence_GetItem(obj, 0);
  if ( !PyFloat_Check(pyOffset) ){
    PyErr_SetString(PyExc_ValueError, "Color stop must start with a floating point offset");
    faint::py_xdecref(pyOffset);
    return false;
  }
  double offset = PyFloat_AsDouble(pyOffset);
  faint::py_xdecref(pyOffset);
  PyObject* pyColor = PySequence_GetItem(obj,1);
  faint::Color c;
  if ( !parse_color(pyColor, c) ){
    faint::py_xdecref(pyColor);
    return false;
  }
  stop = faint::ColorStop(c, offset);
  return true;
}

bool parse_coord( PyObject* args, faint::coord* value ){
  static_assert(ensure_coord_double::value, "faint::coord must be double for PyArg_ParseTuple with \"d\"'");
  if ( !PyArg_ParseTuple( args, "d", value) ){
    return false;
  }
  return true;
}

bool parse_draw_source( PyObject* args, faint::DrawSource& out ){
  faint::Color c;
  if ( parse_color(args, c) ){
    out = faint::DrawSource(c);
    return true;
  }

  // Unwrap if single-item sequence
  PyObject* obj = nullptr;
  bool sequence = false;
  if ( PySequence_Check(args) ){
    sequence = true;
    if ( PySequence_Length(args) != 1 ){
      PyErr_SetString(PyExc_ValueError, "Expected a color tuple, Gradient or Pattern");
      return false;
    }
    obj = PySequence_GetItem(args, 0);
  }
  else {
    obj = args;
  }
  PyErr_Clear();
  faint::Gradient* gradient = as_Gradient(obj);
  faint::Pattern* pattern = nullptr;
  if ( gradient == nullptr ){
    PyErr_Clear();
    pattern = as_Pattern(obj);
  }
  if ( sequence ){
    faint::py_xdecref(obj);
  }

  if ( gradient != nullptr ){
    out = faint::DrawSource(*gradient);
    delete gradient; // Fixme: Tricky business
    return true;
  }
  else if ( pattern != nullptr ){
    out = faint::DrawSource(*pattern);
    return true;
  }
  PyErr_SetString(PyExc_ValueError, "Expected a color tuple, Gradient or Pattern");
  return false;
}

bool parse_point( PyObject* args, Point* out ){
  static_assert(ensure_coord_double::value, "faint::coord must be double for PyArg_ParseTuple with \"d\"'");
  faint::coord x = 0;
  faint::coord y = 0;
  if ( !PyArg_ParseTuple( args, "dd", &x, &y ) ){
    PyErr_Clear();
    if ( !PyArg_ParseTuple( args, "(dd)", &x, &y ) ){
      return false;
    }
  }
  *out = Point(x, y);
  return true;
}

bool parse_point2( PyObject* args, Point* out1, Point* out2 ){
  static_assert(ensure_coord_double::value, "faint::coord must be double for PyArg_ParseTuple with \"d\"'");
  faint::coord x0, y0, x1, y1;
  if ( PyArg_ParseTuple(args, "dddd", &x0, &y0, &x1, &y1) ){
      out1->x = x0;
      out1->y = y0;
      out2->x = x1;
      out2->y = y1;
      return true;
  }
  return false;
}

bool parse_intpoint( PyObject* args, IntPoint* out ){
  int x, y;
  if ( PyArg_ParseTuple(args, "ii", &x, &y) ){
    out->x = x;
    out->y = y;
    return true;
  }
  return false;
}

bool parse_intsize( PyObject* args, IntSize* out ){
  int w, h;
  Py_ssize_t len = PySequence_Length(args);
  if ( len == 1 ){
    if ( !PyArg_ParseTuple(args, "(ii)", &w, &h) ){
      return false;
    }
  }
  else {
    if ( !PyArg_ParseTuple(args, "ii", &w, &h) ){
      return false;
    }
  }
  out->w = w;
  out->h = h;
  return true;
}

bool parse_objects( PyObject* args, objects_t* out ){
  int len = PySequence_Length(args);
  if ( len == 0 ){
    PyErr_SetString(PyExc_ValueError, "No objects specified.");
    return false;
  }

  if ( len == 1 ){
    PyObject* firstArg = PySequence_GetItem(args, 0);
    if ( PySequence_Check(firstArg) ){
      args = firstArg;
      len = PySequence_Length(args);
      if ( len == 0 ){
	PyErr_SetString(PyExc_ValueError, "No objects specified");
	return false;
      }
    }
  }

  std::vector<Object*> objs;
  for ( int i = 0; i != len; i++ ){
    PyObject* pyObj = PySequence_GetItem(args, i);
    smthObject* smthObj = as_smthObject(pyObj);
    if ( smthObj == nullptr ){
      PyErr_SetString(PyExc_ValueError, "Not an object");
      return false;
    }
    objs.push_back(smthObj->obj);
  }
  *out = objs;
  return true;
}

bool parse_png_bitmap( PyObject* args, faint::Bitmap& out ){
  if ( PySequence_Length(args) != 1 ){
    PyErr_SetString( PyExc_TypeError, "A single string argument required.");
    return nullptr;
  }

  PyObject* pngStrPy = PySequence_GetItem( args, 0 );
  if (!PyString_Check( pngStrPy ) ){
    faint::py_xdecref(pngStrPy);
    PyErr_SetString( PyExc_TypeError, "Invalid png-string.");
    return false;
  }

  Py_ssize_t len = PyString_Size( pngStrPy );
  faint::Bitmap bmp( from_png( PyString_AsString(pngStrPy), len ) );
  faint::py_xdecref(pngStrPy);
  out = bmp;
  return true;
}

std::vector<PathPt> parse_points( const char* s ){
  std::stringstream ss( s );

  char controlChar;
  faint::coord curr_x = 0;
  faint::coord curr_y = 0;

  std::vector<PathPt> points;

  while ( ss >> controlChar ){
    if ( controlChar == 'M' ){ // Move to absolute
      faint::coord x, y;
      if ( ss >> x >> y ){
        curr_x = x;
        curr_y = y;
        points.push_back( PathPt( PathPt::MoveTo, x, y ) );
        // Absoolute line-to coordinates may follow
        while ( ss >> x >> y ){
          curr_x = x;
          curr_y = y;
          points.push_back(PathPt ( PathPt::LineTo, x, y ) );
        }
      }
      ss.clear();
    }
    else if ( controlChar == 'm' ){ // Move to relative
      faint::coord x, y;
      if ( ss >> x >> y ){
        curr_x += x;
        curr_y += y;
        points.push_back( PathPt( PathPt::MoveTo, curr_x, curr_y ) );

        // Relative line-to coordinates may follow
        while ( ss >> x >> y ){
          curr_x += x;
          curr_y += y;
          points.push_back(PathPt ( PathPt::LineTo, curr_x, curr_y ) );
        }
      }
      ss.clear();
    }
    else if ( controlChar == 'L' ){ // Line-to absolute
      faint::coord x, y;
      while ( ss >> x >> y ){
        curr_x = x;
        curr_y = y;
        points.push_back( PathPt( PathPt::LineTo, x, y ) );
      }
      ss.clear();
    }
    else if ( controlChar == 'l' ){ // Line-to relative
      faint::coord x, y;
      while( ss >> x >> y ){
        points.push_back( PathPt( PathPt::LineTo, curr_x + x, curr_y + y ) );
        curr_x += x;
        curr_y += y;
      }
      ss.clear();
    }
    else if ( controlChar == 'V' ){
      faint::coord y;
      while ( ss >> y ){
        points.push_back( PathPt( PathPt::LineTo, curr_x, y ) );
        curr_y = y;
      }
      ss.clear();
    }
    else if ( controlChar == 'v' ){
      faint::coord y;
      while ( ss >> y ){
        points.push_back( PathPt( PathPt::LineTo, curr_x, curr_y + y ) );
        curr_y += y;
      }
      ss.clear();
    }
    else if ( controlChar == 'H' ){
      faint::coord x;
      while ( ss >> x ){
        points.push_back( PathPt( PathPt::LineTo, x, curr_y ) );
        curr_x = x;
      }
      ss.clear();
    }
    else if ( controlChar == 'h' ){
      faint::coord x;
      while ( ss >> x ){
        points.push_back( PathPt( PathPt::LineTo, curr_x + x, curr_y ) );
        curr_x += x;
      }
      ss.clear();
    }

    else if ( controlChar == 'C' ){ // Absolute cubic bezier
      faint::coord x0, y0, x1, y1, x2,y2;
      while ( ss >> x0 >> y0 >> x1 >> y1 >> x2 >> y2 ){
        curr_x = x2;
        curr_y = y2;
        points.push_back( PathPt( PathPt::CubicBezier, x2, y2, x0, y0, x1, y1 ) );
      }
      ss.clear();
    }
    else if ( controlChar == 'c' ){ // Relative cubic bezier
      faint::coord x0, y0, x1, y1, x2,y2;
      while ( ss >> x0 >> y0 >> x1 >> y1 >> x2 >> y2 ){
        points.push_back( PathPt( PathPt::CubicBezier, x2 + curr_x, y2 + curr_y, x0 + curr_x, y0 + curr_y, x1 + curr_x, y1 + curr_y ) );
        curr_x += x2;
        curr_y += y2;
      }
      ss.clear();
    }
    else if ( controlChar == 'z' || controlChar == 'Z' ){
      points.push_back( PathPt( PathPt::Close ) );
    }
    else if ( controlChar == 'A' ){
      faint::coord rx, ry, x_axis_rotation, x, y;
      int large_arc_flag, sweep_flag;
      while ( ss >> rx >> ry >>  x_axis_rotation >>  large_arc_flag >>  sweep_flag >>  x >>  y ){
        points.push_back( PathPt::Arc( rx, ry, x_axis_rotation, large_arc_flag, sweep_flag, x, y ) );
        curr_x = x;
        curr_y = y;
      }
      ss.clear();
    }
    else if ( controlChar == 'a' ){
      faint::coord rx, ry, x_axis_rotation, x, y;
      int large_arc_flag, sweep_flag;
      while ( ss >> rx >>  ry >>  x_axis_rotation >>  large_arc_flag >>  sweep_flag >>  x >> y ){
        points.push_back( PathPt::Arc( rx, ry, x_axis_rotation, large_arc_flag, sweep_flag, curr_x + x, curr_y + y ) );
        curr_x += x;
        curr_y += y;
      }
      ss.clear();
    }
  }
  return points;
}

bool parse_radian( PyObject* args, faint::radian* out ){
  return PyArg_ParseTuple( args, "d", out ) != 0;
}

bool parse_radii( PyObject* args, Radii& out ){
  double rx, ry;
  if ( !PyArg_ParseTuple(args, "dd", &rx, &ry ) ){
    return false;
  }
  out.x = rx;
  out.y = ry;
  return true;
}

bool parse_intrect( PyObject* args, IntRect* out ){
  int x, y, w, h;
  if ( PyArg_ParseTuple( args, "iiii", &x, &y, &w, &h ) ){
    out->x = x;
    out->y = y;
    out->w = w;
    out->h = h;
    return true;
  }
  return false;
}

bool parse_rect( PyObject* args, Rect* out ){
  static_assert(ensure_coord_double::value, "faint::coord must be double for PyArg_ParseTuple with \"d\"'");

  faint::coord x, y, w, h;
  if ( PyArg_ParseTuple( args, "dddd", &x, &y, &w, &h ) ){
    out->x = x;
    out->y = y;
    out->w = w;
    out->h = h;
    return true;
  }
  return false;
}

bool parse_rgb_color( PyObject* args, faint::Color& color ){
  return parse_color( args, color, false );
}

bool parse_tri( PyObject* args, Tri* out ){
  faint::coord x0, y0, x1, y1, x2, y2;
  if ( PyArg_ParseTuple( args, "dddddd", &x0, &y0, &x1, &y1, &x2, &y2 ) ){
    *out = Tri(Point(x0, y0), Point(x1, y1), Point(x2, y2) );
    return true;
  }
  return false;
}

faint::PyPoint point_to_py( const Point& src ){
  PyObject* x = PyFloat_FromDouble( src.x );
  PyObject* y = PyFloat_FromDouble( src.y );
  faint::PyPoint point;
  point.x = x;
  point.y = y;
  return point;
}

std::string points_to_svg_path_string( const std::vector<PathPt>& points ){
  std::stringstream ss;
  for ( size_t i = 0; i != points.size(); i++ ){
    const PathPt& pt = points[i];
    if ( PathPt::ArcTo == pt.type ){
      ss << "A" << " " <<
      pt.rx << " " <<
      pt.ry << " " <<
      pt.axis_rotation << " " <<
      pt.large_arc_flag << " " <<
      pt.sweep_flag << " " <<
      pt.x << " " <<
      pt.y << " ";
    }
    else if ( pt.type == PathPt::Close ){
      ss << "z ";
    }
    else if ( PathPt::CubicBezier == pt.type ){
      ss << "C " << pt.cx << "," << pt.cy << " " << pt.dx << "," << pt.dy << " " << pt.x << "," << pt.y << " ";
    }
    else if ( PathPt::LineTo == pt.type ){
      ss << "L " << pt.x << "," << pt.y << " ";
    }
    else if ( PathPt::MoveTo == pt.type ){
      ss << "M " << pt.x << "," << pt.y << " ";
    }
    else {
      assert( false ); // Invalid PathPt type
    }
  }
  std::string pathStr = ss.str();
  return pathStr.substr(0, pathStr.size() - 1 );
}

PyObject* python_bool( bool v ){
  if ( v ){
    Py_RETURN_TRUE;
  }
  Py_RETURN_FALSE;
}

std::string spline_to_svg_path_string( const std::vector<Point>& points ){
  std::stringstream ss;
  const Point& p0 = points[0];
  faint::coord x1 = p0.x;
  faint::coord y1 = p0.y;

  const Point& p1 = points[1];
  faint::coord c = p1.x;
  faint::coord d = p1.y;
  faint::coord x3 = ( x1 + c ) / 2;
  faint::coord y3 = ( y1 + d ) / 2;

  ss << "M " << x1 << "," << y1 << " ";
  ss << "L " << x3 << "," << y3 << " ";

  for ( size_t i = 2; i < points.size(); i++ ){
    const Point& pt = points[i];
    x1 = x3;
    y1 = y3;
    faint::coord x2 = c;
    faint::coord y2 = d;
    c = pt.x;
    d = pt.y;
    x3 = ( x2 + c ) / 2;
    y3 = ( y2 + d ) / 2;
    ss << "C " <<
      x1 << "," << y1 << " " <<
      x2 << "," << y2 << " " <<
      x3 << "," << y3 << " ";
  }

  ss << "L " << c << "," << d;
  return ss.str();
}

PyObject* to_py_png_string( const faint::Bitmap& bmp ){
  std::string pngString( to_png_string( bmp ) );
  PyObject* pythonString = PyString_FromStringAndSize( pngString.c_str(), pngString.size() );
  return pythonString;
}

PyObject* to_py_raw_rgb_string( const faint::Bitmap& bmp ){
  std::string str;
  str.reserve(area(bmp.GetSize())*3);
  for ( int y = 0; y != bmp.m_h; y++ ){
    for ( int x = 0; x != bmp.m_w; x++ ){
      faint::Color c(get_color_raw(bmp, x, y));
      str += c.r;
      str += c.g;
      str += c.b;
    }
  }

  PyObject* py_str = PyString_FromStringAndSize(str.c_str(), str.size());
  return py_str;
}

static std::string get_repr( const faint::Color& c ){
  return bracketed(str_rgba(c));
}

bool coord_from_py_noerr( PyObject* obj, faint::coord& c ){
  if ( !PyNumber_Check(obj) ){
    return false;
  }
  PyObject* pythonFloat = PyNumber_Float(obj);
  c = PyFloat_AsDouble(pythonFloat);
  return true;
}

bool point_from_sequence_noerr( PyObject* obj, Point& p ){
  if ( !PySequence_Check(obj) ){
    return false;
  }
  if ( PySequence_Length(obj) != 2 ){
    return false;
  }
  faint::ScopedRef py_x(PySequence_GetItem(obj, 0));
  faint::coord x = 0.0;
  if ( !coord_from_py_noerr(*py_x, x) ){
    return false;
  }
  faint::ScopedRef py_y(PySequence_GetItem(obj, 1));
  faint::coord y = 0.0;
  if ( !coord_from_py_noerr(*py_y, y) ){
    return false;
  }
  p.x = x;
  p.y = y;
  return true;
}

static std::string get_repr( const Point& pt ){
  std::stringstream ss;
  ss << "(" << pt.x << ", " << pt.y << ")";
  return ss.str();
}

static std::string get_repr( const faint::ColorStop& stop ){
  std::stringstream ss;
  ss << stop.GetOffset() << ", " << get_repr(stop.GetColor());
  return bracketed(ss.str());
}

static std::string get_repr( const faint::color_stops_t& stops ){
  std::stringstream ss;
  ss << "[";
  for ( faint::color_stops_t::const_iterator it = stops.begin(); it != stops.end(); ++it ){
    ss << get_repr(*it);
    if ( it != stops.end() - 1 ){
      ss << ",";
    }
  }
  ss << "]";
  return ss.str();
}

std::string get_repr( const faint::LinearGradient& g ){
  std::stringstream ss;
  ss << "LinearGradient(" << g.GetAngle() << ", " << get_repr(g.GetStops()) << ")";
  return ss.str();
}

std::string get_repr( const faint::RadialGradient& g ){
  std::stringstream ss;
  ss << "RadialGradient(" << get_repr(g.GetCenter()) << ", " << get_repr(g.GetStops()) << ")";
  return ss.str();
}

namespace faint{
  PyObject* unpack_once( PyObject* obj ){
    const int n = PySequence_Length(obj);
    if ( n == 1 ){
      PyObject* item = PySequence_GetItem(obj, 0);
      if ( PySequence_Check(item) && !PyString_Check(item) ){
	return item;
      }
    }
    return obj;
  }

  bool is_string_sequence( PyObject* obj ){
    if ( !PySequence_Check(obj) || PyString_Check(obj) ){
      return false;
    }
    obj = unpack_once(obj);
    const int n = PySequence_Length( obj );
    if ( n == 0 ){
      return false;
    }
    for ( int i = 0; i != n; i++ ){
      PyObject* item = PySequence_GetItem(obj,i);
      if ( !PyString_Check(item) ){
	return false;
      }
    }
    return true;
  }

  std::vector<std::string> parse_string_sequence(PyObject* obj){
    obj = unpack_once(obj);
    assert(!PyString_Check(obj));
    std::vector<std::string> strings;
    const int n = PySequence_Length( obj );
    for ( int i = 0; i != n; i++ ){
      PyObject* item = PySequence_GetItem(obj, i);
      assert(PyString_Check(item));
      const char* data_cstr = PyString_AsString(item); // Fixme: Consider max length
      strings.push_back(std::string(data_cstr));
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
    if ( errType == nullptr ){
      return false;
    }
    PyObject* loadError = get_load_exception_type();
    int result = PyErr_GivenExceptionMatches(errType, loadError);
    return result != 0;
  }

  bool py_save_error_occurred(){
    PyObject* errType = PyErr_Occurred();
    if ( errType == nullptr ){
      return false;
    }
    PyObject* saveError = get_save_exception_type();
    int result = PyErr_GivenExceptionMatches(errType, saveError);
    return result != 0;
  }


  std::string py_error_string(){
    PyObject* type = nullptr;
    PyObject* value = nullptr;
    PyObject* traceBack = nullptr;

    PyErr_Fetch(&type, &value, &traceBack);
    if ( value == nullptr ){
      return "";
    }
    PyObject* exceptionStr = PyObject_Str(value);
    faint::py_xdecref(type);
    faint::py_xdecref(value);
    faint::py_xdecref(traceBack);
    if ( exceptionStr == nullptr ){
      return "";
    }
    char* str(PyString_AsString(exceptionStr));
    std::string errorString(str);
    faint::py_xdecref(exceptionStr);
    return errorString;
  }

  std::vector<std::string> parse_traceback( PyObject* tb ){
    PyObject* moduleName = PyString_FromString("traceback");
    PyObject* tracebackModule = PyImport_Import(moduleName);
    faint::py_xdecref(moduleName);
    assert(tracebackModule != nullptr);
    PyObject* dict = PyModule_GetDict(tracebackModule);
    PyObject* format_tb = PyDict_GetItemString(dict, "format_tb");

    PyObject* args = Py_BuildValue("Oi", tb, 10);
    PyObject* tbList = PyEval_CallObject( format_tb, args );
    assert( PySequence_Check(tbList) );
    int n = PySequence_Length(tbList);
    std::vector<std::string> v;
    v.reserve(n);
    for ( int i = 0; i != n; i++ ){
      PyObject* item = PySequence_GetItem(tbList, i);
      v.push_back(object_string(item));
      faint::py_xdecref(item);
    }
    faint::py_xdecref(tracebackModule);
    return v;
  }

  FaintPyExc py_error_info(){
    PyObject* type = nullptr;
    PyObject* value = nullptr;
    PyObject* traceback = nullptr;
    PyErr_Fetch(&type, &value, &traceback);
    if ( type == nullptr ){
      return FaintPyExc();
    }

    FaintPyExc info;
    info.type = object_string(type);
    info.value = object_string(value);
    info.stackTrace = parse_traceback(traceback);
    return info;
  }

  PyObject* get_load_exception_type(){
    faint::WritableStr typeString("ifaint.LoadError");
    static PyObject* faintPyLoadError = PyErr_NewException(typeString.c_str(),
      nullptr, nullptr);
    return faintPyLoadError;
  }

  PyObject* get_save_exception_type(){
    faint::WritableStr typeString("ifaint.SaveError");
    static PyObject* faintPySaveError = PyErr_NewException(typeString.c_str(),
      nullptr, nullptr);
    return faintPySaveError;
  }

  bool PyRadian_Check( PyObject* obj ){
    return PyFloat_Check(obj);
  }

  radian PyRadian_AsRadian( PyObject* obj ){
    assert(PyRadian_Check(obj));
    return PyFloat_AsDouble(obj);
  }
} // namespace faint
