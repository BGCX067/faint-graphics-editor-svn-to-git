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

#include <algorithm>
#include <cfloat>
#include <cmath>
#include "geo/point.hh"
#include "geo/scale.hh"
#include "util/angle.hh"
#include "util/nan-check.hh"
#include "util/util.hh"
using std::min;
using std::max;
using faint::pi;

Point::Point() :
  x(0),
  y(0)
{}

Point::Point( faint::coord in_x, faint::coord in_y ) :
  x(in_x),
  y(in_y)
{}

bool Point::operator==( const Point& other ) const{
  return coord_eq( x, other.x ) && coord_eq( y, other.y );
}

bool Point::operator!=( const Point& other ) const{
  return !operator==(other);
}

Point Point::operator-() const {
  return Point( -x, -y );
}

// Non-class members
Point operator-( const Point& lhs, const Point& rhs ){
  return Point( lhs.x - rhs.x, lhs.y - rhs.y );
}

Point operator+( const Point& lhs, const Point& rhs ){
  return Point( lhs.x + rhs.x, lhs.y + rhs.y );
}

Point operator*( const Point& p, faint::coord scale ){
  return Point( p.x * scale, p.y * scale );
}

Point operator*( const Point& p0, const Point& p1){
  return Point( p0.x * p1.x, p0.y * p1.y );
}

Point operator/( const Point& p, faint::coord d){
  return Point( p.x/d, p.y/d );
}

Point operator/( const Point& lhs, const Point& rhs ){
  return Point( lhs.x / rhs.x, lhs.y / rhs.y );
}

Point abs( const Point& p ){
  return Point(std::fabs(p.x), std::fabs(p.y));
}

Point max_coords( const Point& p0, const Point& p1 ){
  return Point( std::max( p0.x, p1.x ), std::max( p0.y, p1.y ) );
}

Point max_coords( const Point& p0, const Point& p1, const Point& p2 ){
  return Point( max( max( p0.x, p1.x ), p2.x ), max( max( p0.y, p1.y ), p2.y ) );
}

Point max_coords( const Point& p0, const Point& p1, const Point& p2, const Point& p3 ){
  return Point( max( max( max( p0.x, p1.x ), p2.x ), p3.x), max( max( max( p0.y, p1.y ), p2.y), p3.y) );
}

Point min_coords( const Point& p0, const Point& p1 ){
  return Point( min( p0.x, p1.x ), min( p0.y, p1.y ) );
}

Point min_coords( const Point& p0, const Point& p1, const Point& p2 ){
  return Point( min( min( p0.x, p1.x ), p2.x ), min( min( p0.y, p1.y ), p2.y ) );
}

Point min_coords( const Point& p0, const Point& p1, const Point& p2, const Point& p3 ){
  return Point( min( min( min( p0.x, p1.x ), p2.x ), p3.x), min( min( min( p0.y, p1.y ), p2.y), p3.y) );
}

Point rotate_point( const Point& pt, faint::radian angle, const Point& origin ){
  Point p2 = pt - origin;
  return Point( p2.x * cos(angle) - p2.y * sin(angle),
    p2.x * sin(angle) + p2.y * cos(angle)) + origin;
}

Point scale_point( const Point& pt, const Scale& scale, const Point& origin ){
  Point p2 = pt - origin;
  p2.x *= scale.x;
  p2.y *= scale.y;
  return p2 + origin;
}

Point transposed( const Point& pt ){
  return Point(pt.y, pt.x);
}

bool valid( const Point& p ){
  return !( faint_isnan(p.x) || faint_isnan(p.y) );
}
