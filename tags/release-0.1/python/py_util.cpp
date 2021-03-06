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
#include "pythoninclude.hh"
#include "py_util.hh"
#include "objects/objellipse.hh"
#include "objects/objpath.hh"
#include "objects/objraster.hh"
#include "objects/objspline.hh"
#include "objects/objtext.hh"

bool ParseColor( PyObject* args, faint::Color& color, bool allowAlpha ){
  int r = 255;
  int g = 255;
  int b = 255;
  int a = 255;

  bool ok = PyArg_ParseTuple( args, "iiii", &r, &g, &b, &a ) ||
    PyArg_ParseTuple( args, "iii", &r, &g, &b ) ||
    PyArg_ParseTuple( args, "(iiii)", &r, &g, &b, &a ) ||
    PyArg_ParseTuple( args, "(iii)", &r, &g, &b );

  if ( !ok ){
    PyErr_SetString( PyExc_ValueError, "Invalid color specification. Valid formats for colors are r,g,b and r,g,b,a" );
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

  color = faint::ColorFromInts(r,g,b,a);
  return true;
}

bool ParseColorRGB( PyObject* args, faint::Color& color ){
  return ParseColor( args, color, false );
}

PyObject* build_color_tuple( const faint::Color& color ){
  if ( color.a == 255 ){
    return Py_BuildValue("iii", color.r, color.g, color.b );
  }
  return Py_BuildValue("iiii", color.r, color.g, color.b, color.a );
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
  if ( x < 0 || x >= static_cast<int>(bmp.m_w) || y < 0 || y >= static_cast<int>(bmp.m_h) ){
    PyErr_SetString( PyExc_ValueError, "x,y outside image" );
    return true;
  }
  return false;
}

smthObject* as_smthObject( PyObject* obj ){
  if ( !PyObject_IsInstance( obj, (PyObject*)&SmthType ) ){
    PyErr_SetString(PyExc_ValueError, "Not a Faint Object" );
    return 0;
  }
  return (smthObject*)obj;
}

ObjPath* as_ObjPath( PyObject* obj ){
  smthObject* smthObj = as_smthObject( obj );
  if ( smthObj == 0 ){
    return 0;
  }

  ObjPath* path = dynamic_cast<ObjPath*>( smthObj->obj );
  return path;
}

ObjEllipse* as_ObjEllipse( PyObject* obj ){
  smthObject* smthObj = as_smthObject( obj );
  if ( smthObj == 0 ){
    return 0;
  }
  ObjEllipse* ellipse = dynamic_cast<ObjEllipse*>( smthObj->obj );
  return ellipse;
}

ObjSpline* as_ObjSpline( PyObject* obj ){
  smthObject* smth = as_smthObject( obj );
  if ( smth == 0 ){
    return 0;
  }
  ObjSpline* spline = dynamic_cast<ObjSpline*>( smth->obj );
  return spline;
}

std::pair<CanvasInterface*, ObjText*> as_ObjText( PyObject* obj ){
  smthObject* smth = as_smthObject( obj );
  if ( smth == 0 ){
    return std::make_pair<CanvasInterface*, ObjText*>((CanvasInterface*)0,(ObjText*)0);
  }
  ObjText* text = dynamic_cast<ObjText*> ( smth->obj );
  if ( text == 0 ){
    PyErr_SetString( PyExc_ValueError, "Not a Text object" );
    return std::make_pair<CanvasInterface*, ObjText*>(0,0);
  }
  return std::make_pair(smth->canvas, text);
}

std::pair<CanvasInterface*, ObjRaster*> as_ObjRaster( PyObject* obj ){
  smthObject* smth = as_smthObject( obj );
  if ( smth == 0 ){
    return std::make_pair<CanvasInterface*, ObjRaster*>(0,0);
  }
  ObjRaster* raster = dynamic_cast<ObjRaster*> ( smth->obj );
  if ( raster == 0 ){
    PyErr_SetString( PyExc_ValueError, "Not a Raster object" );
    return std::make_pair<CanvasInterface*, ObjRaster*>(0,0);
  }
  return std::make_pair(smth->canvas, raster);
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

std::string points_to_svg_path_string( const std::vector<PathPt>& points ){
  std::stringstream ss;
  for ( size_t i = 0; i != points.size(); i++ ){
    const PathPt& pt = points[i];
    if ( pt.type == PathPt::MoveTo ){
      ss << "M " << pt.x << "," << pt.y << " ";
    }
    else if ( pt.type == PathPt::LineTo ){
      ss << "L " << pt.x << "," << pt.y << " ";
    }
    else if ( pt.type == PathPt::CubicBezier ){
      ss << "C " << pt.cx << "," << pt.cy << " " << pt.dx << "," << pt.dy << " " << pt.x << "," << pt.y << " ";
    }
    else if ( pt.type == PathPt::Close ){
      ss << "z ";
    }
    else {
      return "";
    }
  }

  std::string pathStr = ss.str();
  return pathStr.substr(0, pathStr.size() - 1 );
}

bool ObjectsFromArgs( PyObject* args, std::vector<Object*>& v ){
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
    if ( obj == 0 ){
      PyErr_SetString(PyExc_ValueError, "That's not a Faint object.");
      return false;
    }
    v.push_back( obj->obj );
  }
  return true;
}

CanvasInterface* as_Canvas( PyObject* obj ){
  if ( !PyObject_IsInstance( obj, (PyObject*)&CanvasType ) ){
    PyErr_SetString(PyExc_ValueError, "Not a Canvas" );
    return 0;
  }
  canvasObject* py_canvas = (canvasObject*)obj;
  return py_canvas->canvas;
}

// Fixme: Used? Safe?
PyObject* PythonBool( bool v ){
  if ( v ){
    Py_RETURN_TRUE;
  }
  Py_RETURN_FALSE;
}

#ifdef FAINT_RVALUE_REFERENCES // assume C++0x
typedef std::is_same<faint::coord, double> ensure_coord_double;
#endif

bool ParsePoint( PyObject* args, Point* out ){
  #ifdef FAINT_RVALUE_REFERENCES // assume C++0x
  static_assert(ensure_coord_double::value, "faint::coord must be double for PyArg_ParseTuple with \"d\"'");
  #endif
  faint::coord x = 0;
  faint::coord y = 0;
  if ( !PyArg_ParseTuple( args, "dd", &x, &y ) ){
    return false;
  }
  *out = Point(x, y);
  return true;
}

bool ParseCoord( PyObject* args, faint::coord* value ){
  #ifdef FAINT_RVALUE_REFERENCES // assume C++0x
  static_assert(ensure_coord_double::value, "faint::coord must be double for PyArg_ParseTuple with \"d\"'");
  #endif  
  if ( !PyArg_ParseTuple( args, "d", value) ){
    return false;
  }
  
  return true;
}

std::vector<PathPt> ParsePoints( const char* s ){
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
    if ( controlChar == 'm' ){ // Move to relative
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
      while ( ss >> rx >>  ry >>  x_axis_rotation >>  large_arc_flag >>  sweep_flag >>  x >>  y ){
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
