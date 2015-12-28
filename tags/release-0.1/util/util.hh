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

#ifndef FAINT_UTIL_HH
#define FAINT_UTIL_HH
#include "flag.hh"
#include "bitmap/bitmap.h"
#include <string>
class Grid;
namespace constrain_dir{
  enum ConstrainDir{ NONE, HORIZONTAL, VERTICAL };
}

using constrain_dir::ConstrainDir;
// Constrains pos to lie on a horizontal or vertical straight line
// intersecting origin.
ConstrainDir ConstrainPos( Point& pos, const Point& origin );
ConstrainDir ConstrainPos( IntPoint& pos, const IntPoint& origin );

void ConstrainPos( IntPoint& p, const IntPoint& origin, ConstrainDir );
void ConstrainPos( Point& p, const Point& origin, ConstrainDir );

unsigned int OrthoDistance( const IntPoint& p1, const IntPoint& p2, ConstrainDir );
void ConstrainProportional( Point& pos, const Size& proportion );

// Return an adjusted version of p2 ("p3") such that the angle between the line
// (origin, p3) and y=0 is the closest to the angle between (origin, p2) and y=0
// in the set [ angle * n + offset ].
Point AdjustTo( const Point& origin, const Point&, int angle, int offset=0 );

// Adjusts like AdjustTo, but allows adding another snap-to angle (altAngle)
Point AdjustToDefault( const Point& origin, const Point&, faint::radian angle, faint::radian altAngle );

Point AdjustTo45( const Point& origin, const Point& );
faint::coord distance( const Point&, const Point& );
faint::coord distance( const IntPoint&, const IntPoint& );

Point MidPoint( const Point&, const Point& );
Point Snap( const Grid&, const Point& );

#include <vector>
class Tri;
std::vector<Point> EllipseAsPath( const Tri& tri0 );

inline std::vector<Point> vector_of( const Point& p0, const Point& p1 ){
  std::vector<Point> v;
  v.push_back(p0);
  v.push_back(p1);
  return v;
}

inline std::vector<Point> vector_of( const Point& p0, const Point& p1, const Point& p2 ){
  std::vector<Point> v;
  v.push_back( p0 );
  v.push_back( p1 );
  v.push_back( p2 );
  return v;
}

faint::Bitmap from_jpg( const char*, size_t len );
faint::Bitmap from_png( const char*, size_t len );
std::string to_png( const faint::Bitmap& );

std::vector<std::string> available_font_facenames();
bool ValidFacename( const std::string& );
faint::Color GetHighlightColor();
#endif
