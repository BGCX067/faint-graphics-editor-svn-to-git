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
#include "geo/geotypes.hh"
#include "geo/pathpt.hh"
#include "util/angle.hh"
#include "util/util.hh"

std::vector<IntPoint> as_line_path( const IntRect& r ){
  std::vector<IntPoint> points;
  points.push_back( r.TopLeft() );
  points.push_back( r.TopRight() );
  points.push_back( r.BottomRight() );
  points.push_back( r.BottomLeft() );
  points.push_back( r.TopLeft());
  return points;
}

IntRect bounding_rect( const std::vector<IntPoint>& pts ){
  assert(!pts.empty());
  const IntPoint& first = pts[0];
  int min_x = first.x;
  int min_y = first.y;
  int max_x = first.x;
  int max_y = first.y;
  for ( size_t i = 1; i != pts.size(); i++ ){
    const IntPoint& pt = pts[i];
    min_x = std::min(min_x, pt.x);
    min_y = std::min(min_y, pt.y);
    max_x = std::max(max_x, pt.x);
    max_y = std::max(max_y, pt.y);
  }
  return IntRect(IntPoint(min_x, min_y), IntPoint(max_x, max_y));
}

Rect bounding_rect( const Point& p0, const Point& p1, const Point& p2 ){
  return Rect( min_coords(p0, p1, p2), max_coords( p0,p1, p2) );
}

std::vector<Point> corners( const Rect& r ){
  std::vector<Point> corners;
  corners.push_back( r.TopLeft() );
  corners.push_back( r.TopRight() );
  corners.push_back( r.BottomLeft() );
  corners.push_back( r.BottomRight() );
  return corners;
}

Point floated( const IntPoint& p ){
  return Point(static_cast<faint::coord>(p.x), static_cast<faint::coord>(p.y));
}

Rect floated( const IntRect& r ){
  return Rect( floated(r.TopLeft()), floated(r.GetSize()) );
}

Size floated( const IntSize& sz ){
  return Size( static_cast<faint::coord>(sz.w), static_cast<faint::coord>(sz.h) );
}

IntRect rect_from_size( const IntSize& sz ){
  return IntRect(IntPoint(0,0), sz);
}

IntPoint truncated( const Point& p ){
  return IntPoint( static_cast<int>(p.x), static_cast<int>(p.y) );
}

IntPoint rounded( const Point& p ){
  return IntPoint( static_cast<int>(p.x + 0.5), static_cast<int>(p.y + 0.5) );
}

PathPt rotate_point( const PathPt& pt, faint::radian angle, const Point& origin ){
  Point p( rotate_point( Point( pt.x, pt.y ), angle, origin ) );
  Point c( rotate_point( Point( pt.cx, pt.cy ), angle, origin ) );
  Point d( rotate_point( Point( pt.dx, pt.dy ), angle, origin ) );
  return PathPt( pt.type, p.x, p.y, c.x, c.y, d.x, d.y  );
}

IntSize rounded( const Size& sz ){
  return IntSize( static_cast<int>(sz.w+0.5), static_cast<int>(sz.h+0.5) );
}

PathPt scale_point( const PathPt& pt, const Scale& sc, const Point& origin ){
  PathPt p2( pt );
  p2.x = ( p2.x - origin.x ) * sc.x + origin.x;
  p2.y = ( p2.y - origin.y ) * sc.y + origin.y;

  p2.cx = ( p2.cx - origin.x ) * sc.x + origin.x;
  p2.cy = ( p2.cy - origin.y ) * sc.y + origin.y;

  p2.dx = ( p2.dx - origin.x ) * sc.x + origin.x;
  p2.dy = ( p2.dy - origin.y ) * sc.y + origin.y;
  return p2;
}

IntSize truncated( const Size& sz ){
  return IntSize( static_cast<int>(sz.w), static_cast<int>(sz.h) );
}

IntRect truncated( const Rect& r ){
  return IntRect( truncated(r.TopLeft() ), truncated( r.GetSize() ) );
}
