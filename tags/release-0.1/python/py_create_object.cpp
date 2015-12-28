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

Object* CreateComposite( PyObject* args ){
  PyErr_SetString(PyExc_ValueError, "Not implemented"); // Fixme
  return 0;
}

Object* CreateEllipse( PyObject* args ){
  faint::coord x, y, w, h;
  settingsObject* pySettings=0;
  if ( ! PyArg_ParseTuple(args, "(dddd)O!", &x, &y, &w, &h, &SettingsType, &pySettings ) ){
    return 0;
  }
  FaintSettings ellipseSettings(GetEllipseSettings());
  ellipseSettings.Update(*(pySettings->settings));
  return new ObjEllipse( TriFromRect(Rect(Point(x,y),Size(w,h))), ellipseSettings );
}

Object* CreateLine( PyObject* args ){
  faint::coord x0, y0, x1, y1;
  settingsObject* pySettings = 0;
  if ( !PyArg_ParseTuple(args, "(dddd)O!", &x0, &y0, &x1, &y1, &SettingsType, &pySettings) ){
    return 0;
  }
  FaintSettings lineSettings(GetLineSettings());
  lineSettings.Update( *pySettings->settings );
  return new ObjLine( Point(x0,y0),Point(x1,y1), lineSettings );
}

Object* CreatePath( PyObject* args ){
  char* s = 0;
  settingsObject* pySettings = 0;
  if ( !PyArg_ParseTuple(args, "sO!", &s, &SettingsType, &pySettings ) ){
    return 0;
  }

  std::vector<PathPt> points( ParsePoints(s) );
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

  FaintSettings pathSettings(GetPathSettings());
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
    PyErr_SetString(PyExc_ValueError, "The first argument must be a sequence" );
  }

  const int n = PySequence_Length( sequence );
  if ( n <= 0 ){
    PyErr_SetString(PyExc_ValueError, "No parameters passed.\nPolygon requires coordinate parameters.");
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
      PyErr_SetString(PyExc_ValueError, "Parameter error" );
      return 0;
    }

    if ( !PyNumber_Check( xItem) ){
      Py_DECREF( xItem );
      PyErr_SetString(PyExc_ValueError, "Non-numeric parameter");
      return 0;
    }
    faint::coord x = PyFloat_AsDouble( xItem );
    Py_DECREF( xItem );

    PyObject* yItem = PySequence_GetItem( sequence, i + 1 );
    if ( yItem == 0 ){
      PyErr_SetString(PyExc_ValueError, "Parameter error" );
      return 0;
    }
    if ( !PyNumber_Check(yItem) ){
      Py_DECREF( yItem );
      PyErr_SetString(PyExc_ValueError, "Non-numeric parameter");
      return 0;
    }

    faint::coord y = PyFloat_AsDouble( yItem );
    Py_DECREF( yItem );
    points.Append( Point( x, y ) );
  }

  FaintSettings polygonSettings(GetPolygonSettings());
  polygonSettings.Update(*(pySettings->settings) );
  return new ObjPolygon( points, polygonSettings );
}

Object* CreateRectangle( PyObject* args ){
  faint::coord x, y, w, h;
  settingsObject* pySettings=0;
  if ( ! PyArg_ParseTuple(args, "(dddd)O!", &x, &y, &w, &h, &SettingsType, &pySettings ) ){
    return 0;
  }
  FaintSettings rectSettings(GetRectangleSettings());
  rectSettings.Update(*(pySettings->settings));
  return new ObjRectangle( TriFromRect(Rect(Point(x,y),Size(w,h))), rectSettings );
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

  FaintSettings rasterSettings(GetRasterSettings());
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
    PyErr_SetString(PyExc_ValueError, "The first argument must be a sequence" );
  }
  const int n = PySequence_Length( sequence );
  if ( n <= 0 ){
    PyErr_SetString(PyExc_ValueError, "Empty sequence.\nSpline requires coordinate parameters.");
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
      PyErr_SetString(PyExc_ValueError, "Parameter error");
      return 0;
    }

    if ( !PyNumber_Check( xItem) ){
      Py_DECREF( xItem );
      PyErr_SetString(PyExc_ValueError, "Non-numeric parameter");
      return 0;
    }
    faint::coord x = PyFloat_AsDouble( xItem );
    Py_DECREF( xItem );

    PyObject* yItem = PySequence_GetItem( sequence, i + 1 );
    if ( yItem == 0 ){
      PyErr_SetString(PyExc_ValueError, "Parameter error");
      return 0;
    }
    if ( !PyNumber_Check(yItem) ){
      Py_DECREF( yItem );
      PyErr_SetString(PyExc_ValueError, "Non-numeric parameter");
      return 0;
    }

    faint::coord y = PyFloat_AsDouble( yItem );
    points.Append( Point( x, y ) );
  }

  FaintSettings splineSettings( GetSplineSettings() );
  splineSettings.Update( *(pySettings->settings) );
  return new ObjSpline( points, splineSettings );
}

bool to_wxString2( PyObject* str_py, wxString& str_wx ){
  if ( PyUnicode_Check( str_py ) ){
    size_t len = PyUnicode_GET_SIZE( str_py );
    PyUnicode_AsWideChar( (PyUnicodeObject*)str_py, wxStringBuffer(str_wx, len),len );
    return true;
  }
  else if ( PyString_Check( str_py ) ) {
    const char* c_str = PyString_AS_STRING(str_py);
    str_wx = wxString(c_str);
    return true;
  }
  PyErr_SetString(PyExc_TypeError, "string or unicode type required." );
  return false;
}

Object* CreateText( PyObject* args ){
  faint::coord x, y, w, h;
  settingsObject* pySettings=0;
  PyObject* str = 0;
  if ( ! PyArg_ParseTuple(args, "(dddd)OO!", &x, &y, &w, &h, &str, &SettingsType, &pySettings ) ){
    return 0;
  }

  wxString str_wx;
  bool convertOk = to_wxString2( str, str_wx ); // FIXME: Remove to_wxString2
  if ( !convertOk ){
    return 0;
  }

  FaintSettings textSettings(GetTextSettings());
  textSettings.Update(*(pySettings->settings));
  // Fixme: Use Tri
  return new ObjText(Rect(Point(x,y),Size(w,h)), str_wx, textSettings );
}
