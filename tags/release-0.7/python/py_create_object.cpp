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

#include "py_create_object.hh"
#include "py_settings.hh"
#include "py_util.hh"
#include "geo/points.hh"
#include "objects/objellipse.hh"
#include "objects/objline.hh"
#include "objects/objpath.hh"
#include "objects/objpolygon.hh"
#include "objects/objraster.hh"
#include "objects/objtext.hh"
#include "objects/objspline.hh"
#include "objects/objrectangle.hh"
#include "util/convertwx.hh"
#include "util/util.hh"
#include "util/settingutil.hh"

Object* CreateEllipse( PyObject* args ){
  faint::coord x, y, w, h;
  settingsObject* pySettings=0;
  if ( ! PyArg_ParseTuple(args, "(dddd)O!", &x, &y, &w, &h, &SettingsType, &pySettings ) ){
    return 0;
  }
  Settings ellipseSettings(default_ellipse_settings());
  ellipseSettings.Update(*(pySettings->settings));
  return new ObjEllipse( tri_from_rect(Rect(Point(x,y),Size(w,h))), ellipseSettings );
}

Object* CreatePolyLine( PyObject* args ){
  PyObject* sequence = 0;
  settingsObject* pySettings = 0;
  if ( !PyArg_ParseTuple(args, "OO!", &sequence, &SettingsType, &pySettings) ){
    return 0;
  }
  if ( !PySequence_Check(sequence) ){
    PyErr_SetString(PyExc_TypeError, "Bad poly line!"); // Fixme
    return 0;
  }
  const int n = PySequence_Length( sequence );
  if ( n <= 0 ){
    PyErr_SetString(PyExc_TypeError, "No parameters passed.\nLine requires coordinate parameters.");
    return 0;
  }
  if ( n % 2 != 0 ){
    PyErr_SetString( PyExc_ValueError, "Number of parameters is not even.");
    return 0;
  }

  Points points;
  for ( int i = 0; i != n; i+=2 ){
    PyObject* xItem = PySequence_GetItem( sequence, i );
    if ( xItem == 0 ){
      PyErr_SetString(PyExc_TypeError, "Parameter error" );
      return 0;
    }

    if ( !PyNumber_Check( xItem) ){
      faint::py_xdecref( xItem );
      PyErr_SetString(PyExc_TypeError, "Non-numeric parameter");
      return 0;
    }
    faint::coord x = PyFloat_AsDouble( xItem );
    faint::py_xdecref( xItem );

    PyObject* yItem = PySequence_GetItem( sequence, i + 1 );
    if ( yItem == 0 ){
      PyErr_SetString(PyExc_TypeError, "Parameter error" );
      return 0;
    }
    if ( !PyNumber_Check(yItem) ){
      faint::py_xdecref( yItem );
      PyErr_SetString(PyExc_TypeError, "Non-numeric parameter");
      return 0;
    }

    faint::coord y = PyFloat_AsDouble( yItem );
    faint::py_xdecref( yItem );
    points.Append( Point( x, y ) );
  }

  Settings lineSettings(default_line_settings());
  lineSettings.Update(*(pySettings->settings) );
  return new ObjLine( points, lineSettings );
}

Object* CreateLine( PyObject* args ){
  faint::coord x0, y0, x1, y1;
  settingsObject* pySettings = 0;
  if ( !PyArg_ParseTuple(args, "(dddd)O!", &x0, &y0, &x1, &y1, &SettingsType, &pySettings) ){
    PyErr_Clear();
    return CreatePolyLine(args);
  }
  Settings lineSettings(default_line_settings());
  lineSettings.Update( *pySettings->settings );
  return new ObjLine( Points(vector_of(Point(x0,y0),Point(x1,y1))), lineSettings );
}

Object* CreatePath( PyObject* args ){
  char* s = 0;
  settingsObject* pySettings = 0;
  if ( !PyArg_ParseTuple(args, "sO!", &s, &SettingsType, &pySettings ) ){
    return 0;
  }

  std::vector<PathPt> points( parse_points(s) );
  if ( points.empty() ){
    PyErr_SetString(PyExc_ValueError, "Failed parsing Path Definition.");
    return 0;
  }
  if ( points.front().type != PathPt::MoveTo ){
    PyErr_SetString(PyExc_ValueError, "Paths must begin with a Move-entry.");
    return 0;
  }
  if ( points.size() == 1 ){
    PyErr_SetString(PyExc_ValueError, "Paths must contain more than one point.");
    return 0;
  }

  Settings pathSettings(default_path_settings());
  pathSettings.Update(*(pySettings->settings));
  return new ObjPath( Points(points), pathSettings );
}

Object* CreatePolygon( PyObject* args ){
  PyObject* sequence = 0;
  settingsObject* pySettings = 0;
  if ( !PyArg_ParseTuple(args, "OO!", &sequence, &SettingsType, &pySettings ) ){
    return 0;
  }

  if ( !PySequence_Check(sequence) ){
    PyErr_SetString(PyExc_TypeError, "The first argument must be a sequence" );
  }

  const int n = PySequence_Length( sequence );
  if ( n <= 0 ){
    PyErr_SetString(PyExc_TypeError, "No parameters passed.\nPolygon requires coordinate parameters.");
    return 0;
  }
  if ( n % 2 != 0 ){
    PyErr_SetString( PyExc_ValueError, "Number of parameters is not even.");
    return 0;
  }

  Points points;
  for ( int i = 0; i != n; i+=2 ){
    PyObject* xItem = PySequence_GetItem( sequence, i );
    if ( xItem == 0 ){
      PyErr_SetString(PyExc_TypeError, "Parameter error" );
      return 0;
    }

    if ( !PyNumber_Check( xItem) ){
      faint::py_xdecref( xItem );
      PyErr_SetString(PyExc_TypeError, "Non-numeric parameter");
      return 0;
    }
    faint::coord x = PyFloat_AsDouble( xItem );
    faint::py_xdecref( xItem );

    PyObject* yItem = PySequence_GetItem( sequence, i + 1 );
    if ( yItem == 0 ){
      PyErr_SetString(PyExc_TypeError, "Parameter error" );
      return 0;
    }
    if ( !PyNumber_Check(yItem) ){
      faint::py_xdecref( yItem );
      PyErr_SetString(PyExc_TypeError, "Non-numeric parameter");
      return 0;
    }

    faint::coord y = PyFloat_AsDouble( yItem );
    faint::py_xdecref( yItem );
    points.Append( Point( x, y ) );
  }

  Settings polygonSettings(default_polygon_settings());
  polygonSettings.Update(*(pySettings->settings) );
  return new ObjPolygon( points, polygonSettings );
}

Object* CreateRectangle( PyObject* args ){
  faint::coord x, y, w, h;
  settingsObject* pySettings=0;
  if ( ! PyArg_ParseTuple(args, "(dddd)O!", &x, &y, &w, &h, &SettingsType, &pySettings ) ){
    return 0;
  }
  Settings rectSettings(default_rectangle_settings());
  rectSettings.Update(*(pySettings->settings));
  return new ObjRectangle( tri_from_rect(Rect(Point(x,y),Size(w,h))), rectSettings );
}

Object* CreateRaster( PyObject* args ){
  faint::coord x, y, w, h;
  char* type_cstr;
  PyObject* o_data_str;
  settingsObject* pySettings=0;
  if ( !PyArg_ParseTuple( args, "(dddd)sOO!", &x, &y, &w, &h, &type_cstr, &o_data_str, &SettingsType, &pySettings) ){
    return 0;
  }

  if ( !PyString_Check(o_data_str) ){
    PyErr_SetString( PyExc_ValueError, "Invalid data string" );
    return 0;
  }

  Settings rasterSettings(default_raster_settings());
  rasterSettings.Update(*(pySettings->settings));

  Py_ssize_t len = PyString_Size(o_data_str);
  const char* data_cstr = PyString_AsString(o_data_str);
  std::string type(type_cstr);
  if ( type == "jpeg" ){
    return new ObjRaster(Rect(Point(x,y),Size(w,h)), from_jpg(data_cstr, len), rasterSettings);
  }
  else if ( type == "png" ){
    return new ObjRaster(Rect(Point(x,y),Size(w,h)), from_png(data_cstr, len), rasterSettings);
  }

  PyErr_SetString( PyExc_ValueError, "Invalid image type.");
  return 0;
}

Object* CreateSpline( PyObject* args ){
  PyObject* sequence;
  settingsObject* pySettings=0;
  if ( ! PyArg_ParseTuple(args, "OO!", &sequence, &SettingsType, &pySettings ) ){
    return 0;
  }

  if ( !PySequence_Check(sequence) ){
    PyErr_SetString(PyExc_TypeError, "The first argument must be a sequence" );
  }
  const int n = PySequence_Length( sequence );
  if ( n <= 0 ){
    PyErr_SetString(PyExc_TypeError, "Empty sequence.\nSpline requires coordinate parameters.");
    return 0;
  }
  if ( n % 2 != 0 ){
    PyErr_SetString( PyExc_ValueError, "Number of parameters is not even.");
    return 0;
  }

  Points points;
  for ( int i = 0; i != n; i+=2 ){
    PyObject* xItem = PySequence_GetItem( sequence, i );
    if ( xItem == 0 ){
      PyErr_SetString(PyExc_TypeError, "Parameter error");
      return 0;
    }

    if ( !PyNumber_Check( xItem) ){
      faint::py_xdecref( xItem );
      PyErr_SetString(PyExc_TypeError, "Non-numeric parameter");
      return 0;
    }
    faint::coord x = PyFloat_AsDouble( xItem );
    faint::py_xdecref( xItem );

    PyObject* yItem = PySequence_GetItem( sequence, i + 1 );
    if ( yItem == 0 ){
      PyErr_SetString(PyExc_TypeError, "Parameter error");
      return 0;
    }
    if ( !PyNumber_Check(yItem) ){
      faint::py_xdecref( yItem );
      PyErr_SetString(PyExc_TypeError, "Non-numeric parameter");
      return 0;
    }

    faint::coord y = PyFloat_AsDouble( yItem );
    points.Append( Point( x, y ) );
  }

  Settings splineSettings( default_spline_settings() );
  splineSettings.Update( *(pySettings->settings) );
  return new ObjSpline( points, splineSettings );
}

bool py_to_cpp_string( PyObject* str_py, std::string& str ){
  if ( PyUnicode_Check( str_py ) ){
    PyObject* utf8 = PyUnicode_AsUTF8String(str_py);
    const char* c_str = PyString_AsString(utf8); // Pointer to internal data
    str = c_str;
    faint::py_xdecref(utf8);
    return true;
  }
  else if ( PyString_Check( str_py ) ){
    const char* c_str = PyString_AS_STRING(str_py);
    str = c_str;
    return true;
  }
  PyErr_SetString(PyExc_TypeError, "string or unicode type required." );
  return false;
}

Object* CreateText( PyObject* args ){
  faint::coord x, y, w, h;
  settingsObject* pySettings=0;
  PyObject* pystr = 0;
  if ( ! PyArg_ParseTuple(args, "(dddd)OO!", &x, &y, &w, &h, &pystr, &SettingsType, &pySettings ) ){
    return 0;
  }

  std::string str;
  bool convertOk = py_to_cpp_string( pystr, str );
  if ( !convertOk ){
    return 0;
  }

  Settings textSettings(default_text_settings());
  textSettings.Update(*(pySettings->settings));
  return new ObjText(Rect(Point(x,y),Size(w,h)), faint::utf8_string(str), textSettings );
}
