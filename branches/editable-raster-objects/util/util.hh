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
#include <string>
#include <vector>
#include "flag.hh"
#include "commonfwd.hh"

// Return an adjusted version of p2 ("p3") such that the angle between the line
// (origin, p3) and y=0 is the closest to the angle between (origin, p2) and y=0
// in the set [ angle * n + offset ].
Point adjust_to( const Point& origin, const Point&, int angle, int offset=0 ); // Fixme: Origin and point order does not match other functions

// Returns the point adjusted to 45-degrees relative to origin
Point adjust_to_45( const Point& origin, const Point& );

// Adjusts like AdjustTo, but allows adding another snap-to angle (altAngle)
Point adjust_to_default( const Point& origin, const Point&, faint::radian angle, faint::radian altAngle );

std::vector<std::string> available_font_facenames(); // Implemented in wxutil

namespace constrain_dir{
  enum ConstrainDir{ NONE, HORIZONTAL, VERTICAL };
}

using constrain_dir::ConstrainDir;
// Constrains pos to lie on a horizontal or vertical straight line
// intersecting origin.
ConstrainDir constrain_pos( Point&, const Point& origin );
ConstrainDir constrain_pos( IntPoint&, const IntPoint& origin );
void constrain_pos( IntPoint&, const IntPoint& origin, ConstrainDir );
void constrain_pos( Point&, const Point& origin, ConstrainDir );
void constrain_proportional( Point&, const Size& proportion );
Point constrain_proportional( const Point& moved, const Point& opposite, const Point& origin );
std::string convert_wide_to_utf8( const wchar_t& ); // Implemented in wxutil
faint::coord distance( const Point&, const Point& );
faint::coord distance( const IntPoint&, const IntPoint& );

// Returns a vector of cubic bezier spline control points and end points approximating
// the ellipse described by the Tri
std::vector<Point> ellipse_as_path( const Tri& );

faint::Bitmap from_jpg( const char*, size_t len );
faint::Bitmap from_png( const char*, size_t len );
std::string get_default_font_name();
int get_default_font_size();
faint::Color get_highlight_color(); // Implemented in wxutil

// Returns LEFT_MOUSE if the flags contain LEFT_MOUSE, otherwise
// RIGHT_MOUSE
int mbtn(int flags);
Point mid_point( const Point&, const Point& );
std::vector<Point> mid_points( const std::vector<Point>& );
unsigned int ortho_distance( const IntPoint&, const IntPoint&, ConstrainDir );
std::vector<Point> rectangle_as_clockwise_polygon( const Tri& );
Point snap_to_grid( const Grid&, const Point& );

// Returns a string representing the Bitmap encoded as a PNG
std::string to_png_string( const faint::Bitmap& ); // Implemented in wxutil
bool valid_facename( const std::string& ); // Implemented in wxutil

template<typename CONTAINER>
typename CONTAINER::value_type& ultimate( CONTAINER& v ){
  assert(!v.empty());
  return v[ v.size() - 1];
}

template<typename CONTAINER>
typename CONTAINER::value_type& penultimate( CONTAINER& v ){
  assert(v.size() > 1);
  return v[v.size() - 2];
}

template <typename T>
inline std::vector<T> vector_of( const T& e0, const T& e1 ){
  std::vector<T> v;
  v.push_back(e0);
  v.push_back(e1);
  return v;
}

inline std::vector<Point> vector_of( const Point& p ){
  std::vector<Point> v;
  v.push_back(p);
  return v;
}

inline std::vector<Point> vector_of( const Point& p0, const Point& p1, const Point& p2 ){
  std::vector<Point> v;
  v.push_back( p0 );
  v.push_back( p1 );
  v.push_back( p2 );
  return v;
}

std::vector<Point> transform_points( const std::vector<Point>&, faint::coord scale, const Point& origin );
std::vector<IntPoint> floored( const std::vector<Point>& );

#endif
