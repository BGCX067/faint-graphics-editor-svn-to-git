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
#include <utility>
#include <vector>
#include "bitmap/bitmap.h"
#include "geo/geotypes.hh"
#include "geo/points.hh"
#include "py_canvas.hh"
#include "py_something.hh"
class ObjRaster;
class ObjText;
class ObjSpline;
class ObjPath;
class PathPt;
class ObjEllipse;

bool ParseColor( PyObject* sequence, faint::Color&, bool allowAlpha=true );
bool ParseColorRGB( PyObject* sequence, faint::Color& );

PyObject* build_color_tuple( const faint::Color& );
bool invalid_color( int r, int g, int b, int a=255);
bool invalid_pixel_pos( int x, int y, const faint::Bitmap&);

std::string spline_to_svg_path_string( const std::vector<Point>& );
std::string points_to_svg_path_string( const std::vector<PathPt>& );

std::pair<CanvasInterface*, ObjRaster*> as_ObjRaster( PyObject* );
std::pair<CanvasInterface*, ObjText*> as_ObjText( PyObject* );
ObjSpline* as_ObjSpline( PyObject* );
ObjPath* as_ObjPath( PyObject* );
ObjEllipse* as_ObjEllipse( PyObject* );
smthObject* as_smthObject( PyObject* );
bool ObjectsFromArgs( PyObject* args, std::vector<Object*>& );
CanvasInterface* as_Canvas( PyObject* );
PyObject* PythonBool( bool );

bool ParsePoint( PyObject* args, Point* out );
bool ParseCoord( PyObject* args, faint::coord* out );

std::vector<PathPt> ParsePoints( const char* s );
#endif
