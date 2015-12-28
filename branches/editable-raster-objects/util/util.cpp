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

#include <algorithm> // transform
#include <cassert>
#include <cmath>
#include <iterator> // back_inserter
#include "angle.hh"
#include "geo/geotypes.hh"
#include "geo/grid.hh"
#include "geo/tri.hh"
#include "tools/tool.hh" // For LEFT/RIGHT
#include "util.hh"

using faint::pi;

Point adjust_to( const Point& o, const Point& p, int step, int offset ){
  int n_angles = 360 / step + 1;
  faint::coord radius_ = radius( o.x, o.y, p.x, p.y );
  faint::degree angle_ = angle360( o.x, o.y, p.x, p.y );

  for ( int i = 0; i!= n_angles; i++ ){
    faint::degree i_angle = static_cast<faint::degree>( i * step + offset );
    if ( fabs( angle_ - i_angle ) <= step / 2.0 ){
      faint::degree r_newAngle = (360 - i_angle) / ( 180.0 / pi );
      faint::coord x_mplr = cos( r_newAngle );
      faint::coord y_mplr = sin( r_newAngle );

      faint::coord x = x_mplr * radius_ + o.x;
      faint::coord y = y_mplr * radius_ + o.y;
      return Point( x, y );
    }
  }
  return p;
}

Point adjust_to_45( const Point& o, const Point& p ){
  faint::degree angle_ = angle360( o.x, o.y, p.x, p.y );
  faint::coord dx = p.x - o.x;
  faint::coord dy = p.y - o.y;

  if ( 360 - 22.5 < angle_ || angle_ <= 22.5 ){
    return Point( o.x + dx, o.y );
  }
  else if ( 22.5 < angle_ && angle_ <= 67.5 ){
    return Point( o.x - dy, o.y + dy );
  }
  else if ( 67.5 < angle_ && angle_ <= 110.5 ){
    return Point( o.x, o.y + dy );
  }
  else if ( 110.5 < angle_ && angle_ <= 157.5 ){
    return Point( o.x + dy, o.y + dy );
  }
  else if ( 157.5 < angle_ && angle_ <= 202.5 ){
    return Point( o.x + dx, o.y );
  }
  else if ( 202.5 < angle_ && angle_ <= 247.5 ){
    return Point( o.x - dy, o.y + dy );
  }
  else if ( 247.5 < angle_ && angle_ <= 292.5 ){
    return Point( o.x, o.y + dy );
  }
  else if ( 292.5 < angle_ && angle_ < 360 - 22.5 ){
    return Point( o.x + dy, o.y + dy );
  }
  return p;
}

Point adjust_to_default( const Point& origin, const Point& p, faint::radian angle, faint::radian altAngle ){
  faint::radian twoPi = 2 * pi;
  int n_angles = static_cast<int>(twoPi / angle) + 1;
  faint::coord length = radius( origin.x, origin.y, p.x, p.y );
  faint::coord initialAngle = -rad_angle( origin.x, origin.y, p.x, p.y );
  if ( initialAngle < 0 ){
    initialAngle = twoPi + initialAngle;
  }

  faint::radian foundAngle = initialAngle;
  for ( int i = 0; i!= n_angles; i++ ){
    faint::radian thisAngle = i * angle;
    if ( std::fabs( faint::radian(initialAngle - thisAngle) ) <= angle / 2 ){
      foundAngle = thisAngle;
      break;
    }
  }

  if ( std::fabs( initialAngle - altAngle ) < std::fabs( initialAngle - foundAngle ) ){
    // Use the altAngle instead
    foundAngle = altAngle;
  }

  faint::radian newAngle = -foundAngle;
  faint::coord x_mplr = cos( newAngle );
  faint::coord y_mplr = sin( newAngle );
  faint::coord x = x_mplr * length + origin.x;
  faint::coord y = y_mplr * length + origin.y;
  return Point( x, y );
}

ConstrainDir constrain_pos( IntPoint& p, const IntPoint& origin ){
  if ( std::abs(origin.x - p.x) > std::abs(origin.y - p.y) ){
    p.y = origin.y;
    return constrain_dir::HORIZONTAL;
  }
  else {
    p.x = origin.x;
    return constrain_dir::VERTICAL;
  }
}

ConstrainDir constrain_pos( Point& p, const Point& origin ){
  if ( std::fabs(origin.x - p.x) > std::fabs(origin.y - p.y) ){
    p.y = origin.y;
    return constrain_dir::HORIZONTAL;
  }
  else {
    p.x = origin.x;
    return constrain_dir::VERTICAL;
  }
}

void constrain_pos( Point& p, const Point& origin, ConstrainDir dir ){
  if ( dir == constrain_dir::HORIZONTAL ){
    p.y = origin.y;
  }
  else if ( dir == constrain_dir::VERTICAL ){
    p.x = origin.x;
  }
}

void constrain_pos( IntPoint& p, const IntPoint& origin, ConstrainDir dir ){
  if ( dir == constrain_dir::HORIZONTAL ){
    p.y = origin.y;
  }
  else if ( dir == constrain_dir::VERTICAL ){
    p.x = origin.x;
  }
}

void constrain_proportional( Point& pos, const Size& prop ){
  assert( prop.w != 0 );
  assert( prop.h > 0 );
  faint::coord heightProp = prop.h / prop.w;

  // Fixme: Maybe consider checking pos.y / heightProp, pos.y too,
  // And using the closest.
  pos = Point( pos.x, pos.x * heightProp );
}

Point constrain_proportional(const Point& moved, const Point& opposite, const Point& origin){
  // Moved point must lie on the diagonal between opposite and origin
  faint::coord dx = origin.x - opposite.x;
  faint::coord dy = origin.y - opposite.y;
  faint::coord k = dy / dx;
  faint::coord m = origin.y - k * origin.x;
  return Point( moved.x, moved.x * k + m ); // Fixme: consider alternative, constraining x  by y.
}

faint::coord distance( const Point& pt1, const Point& pt2 ){
  return radius( pt1.x, pt1.y, pt2.x, pt2.y );
}

faint::coord distance( const IntPoint& pt1, const IntPoint& pt2 ){
  return distance( floated(pt1), floated(pt2) );
}

std::vector<Point> ellipse_as_path( const Tri& tri0 ){
  faint::coord skew = tri0.Skew();
  Tri tri( skewed( tri0, -skew ) );
  faint::radian angle = tri.Angle();
  tri = rotated( tri, - angle, tri.P0() );

  faint::coord x = tri.P0().x;
  faint::coord y = tri.P0().y;
  faint::coord dx = tri.Width();
  faint::coord dy = tri.Height();

  faint::coord rx = dx / 2.0;
  faint::coord ry = dy / 2.0;
  faint::coord t = 0.551784;

  faint::coord skew_1 = 0;
  faint::coord skew_2 = skew * ( 0.5 - 0.5 * t );
  faint::coord skew_3 = skew * 0.5;
  faint::coord skew_4 = skew * ( 0.5 + 0.5 * t );
  faint::coord skew_5 = skew;

  std::vector<Point> v;
  Point origin(x,y);

  v.push_back( rotate_point( Point( x + rx + skew_1, y + dy ), angle, origin ) ); // Start

  // 1
  v.push_back( rotate_point( Point( x + rx + rx * t + skew_1, y + dy ), angle, origin ) ); // c
  v.push_back( rotate_point( Point( x + dx + skew_2, y + ry + ry * t ), angle, origin ) ); // d
  v.push_back( rotate_point( Point( x + dx + skew_3, y + ry ), angle, origin ) ); // dst

  // 2
  v.push_back( rotate_point( Point( x + dx + skew_4, y + ry - ry * t ), angle, origin ) ); // c
  v.push_back( rotate_point( Point( x + rx + rx * t + skew_5, y ), angle, origin ) ); // d
  v.push_back( rotate_point( Point( x + rx + skew_5, y ), angle, origin ) ); // dst

  // 3
  v.push_back( rotate_point( Point( x + rx - rx * t + skew_5, y ), angle, origin ) ); // c
  v.push_back( rotate_point( Point( x + skew_4, y + ry - ry * t ), angle, origin ) ); // d
  v.push_back( rotate_point( Point( x + skew_3, y + ry ), angle, origin ) ); // dst

  // 4
  v.push_back( rotate_point( Point( x + skew_2, y + ry + ry * t ), angle, origin ) ); // c
  v.push_back( rotate_point( Point( x + rx - rx * t, y + dy ), angle, origin ) ); // d
  v.push_back( rotate_point( Point( x + rx + skew_1, y + dy ), angle, origin ) ); // dst

  return v;
}

bool fl( int flag, int bits ){
  return ( bits & flag ) == flag;
}

int mbtn( int flags ){
  if ( fl( LEFT_MOUSE, flags ) ){
    return LEFT_MOUSE;
  }
  else {
    return RIGHT_MOUSE;
  }
}

Point mid_point( const Point& p0, const Point& p1 ){
  return Point( std::min( p0.x, p1.x ) + std::fabs(p0.x - p1.x) / 2,
    std::min( p0.y, p1.y ) + std::abs(p0.y - p1.y) / 2 );
}

std::vector<Point> mid_points( const std::vector<Point>& in_pts ){
  if ( in_pts.empty() ){
    return std::vector<Point>();
  }

  std::vector<Point> pts(in_pts);
  Point a = pts[0];
  std::vector<Point> midPts;
  for ( size_t i = 1; i != pts.size(); i++ ){
    Point b = pts[i];
    Point p0 = min_coords(a,b);
    Point p1 = max_coords(a,b);
    midPts.push_back(p0 + ( p1 - p0 ) / 2);
    a = pts[i];
  }
  return midPts;
}

unsigned int ortho_distance( const IntPoint& p1, const IntPoint& p2, ConstrainDir dir ){
  unsigned int distance = 0;
  if ( dir == constrain_dir::HORIZONTAL ){
    distance = std::abs(p1.x - p2.x );
  }
  else if ( dir == constrain_dir::VERTICAL ){
    distance = std::abs( p1.y - p2.y );

  }
  else {
    assert( false );
  }
  return distance;
}

std::vector<Point> rectangle_as_clockwise_polygon( const Tri& tri ){
  std::vector<Point> points;
  points.push_back( tri.P0() );
  points.push_back( tri.P1() );
  points.push_back( tri.P3() );
  points.push_back( tri.P2() );
  return points;
}

inline faint::coord round_up( faint::coord c ){
  return static_cast<faint::coord>( static_cast<int>(c + 0.5) );
}

Point snap_to_grid( const Grid& g, const Point& p ){
  Point relativeGrid = p / static_cast<faint::coord>(g.Spacing());
  faint::coord x = round_up( relativeGrid.x ) * g.Spacing();
  faint::coord y = round_up( relativeGrid.y ) * g.Spacing();
  return Point( x, y );
}

IntPoint floor_point(const Point& pt ){ // Boo! Required, or std::transform could not deduce overload.
  return floored(pt);
}

std::vector<IntPoint> floored( const std::vector<Point>& points ){
  std::vector<IntPoint> v2;
  v2.reserve(points.size());
  std::transform(points.begin(), points.end(), std::back_inserter(v2), floor_point );
  return v2;
}

std::vector<Point> transform_points( const std::vector<Point>& points, faint::coord scale, const Point& origin ){
  std::vector<Point> v2;
  v2.reserve(points.size());
  for ( const Point& pt : points ){
    v2.push_back( pt * scale + origin );
  }
  return v2;
}
