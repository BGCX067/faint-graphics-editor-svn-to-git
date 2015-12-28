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

#include <cstddef>
#include "points.hh"

Tri TriFromPoints( const std::vector<PathPt>& points ){
  if ( points.empty() ){
    return Tri( Point(0,0), 0, 0, 0 );
  }
  const PathPt& p0 = points[0];
  faint::coord x_min = p0.x;
  faint::coord x_max = p0.x;
  faint::coord y_min = p0.y;
  faint::coord y_max = p0.y;

  for ( size_t i = 0; i != points.size(); i++ ){
    const PathPt& pt = points[i];
    if ( pt.type == PathPt::Close ){
      break;
    }
    x_min = std::min( x_min, pt.x );
    x_max = std::max( x_max, pt.x );
    y_min = std::min( y_min, pt.y );
    y_max = std::max( y_max, pt.y );
  }
  return Tri( Point( x_min, y_min ), Point(x_max, y_min), Point(x_min, y_max) );
}

Points::Points()
  : m_tri( Point(0,0), 0, 0, 0 )
{}

Points::Points( const Points& other )
  : m_tri( other.m_tri ),
    m_points( other.m_points )
{}

Points::Points( const std::vector<PathPt>& points ) :
  m_tri( TriFromPoints( points ) ),
  m_points(points)
{}

Points::Points( const std::vector<Point>& points ){
  for ( size_t i = 0; i != points.size(); i++ ){
    const Point& p( points[i] );
    m_points.push_back( PathPt( PathPt::LineTo, p.x, p.y ) );
  }
  m_tri = TriFromPoints( m_points );
}

std::vector<PathPt> Points::GetPoints( const Tri& tri ) const{
  Adj a = GetAdj( m_tri, tri );
  std::vector<PathPt> points;
  faint::coord p2y = m_tri.P2().y;
  faint::coord ht = m_tri.Height();
  for ( size_t i = 0; i != m_points.size(); i++ ){
    PathPt pt = m_points[i];
    pt.x += a.skew * ( ( p2y - pt.y ) / ht ) / a.scale_x; // assumes m_tri not rotated
    pt.x += a.tr_x;
    pt.y += a.tr_y;

    pt.cx += a.skew * ( ( p2y - pt.cy ) / ht ) / a.scale_x; // assumes m_tri not rotated
    pt.cx += a.tr_x;
    pt.cy += a.tr_y;

    pt.dx += a.skew * ( ( p2y - pt.dy ) / ht ) / a.scale_x; // assumes m_tri not rotated
    pt.dx += a.tr_x;
    pt.dy += a.tr_y;

    pt = ScalePoint( pt, a.scale_x, a.scale_y, tri.P3() );
    pt = RotatePoint( pt, tri.Angle(), tri.P3() );

    points.push_back( pt );
  }
  return points;
}

std::vector<PathPt> Points::GetPoints() const{
  return m_points;
}

std::vector<Point> Points::GetPointsDumb() const{
  std::vector<Point> v;
  for ( size_t i = 0; i != m_points.size(); i++ ){
    const PathPt& pt( m_points[i] );
    v.push_back( Point( pt.x, pt.y ) );
  }
  return v;
}

std::vector<Point> Points::GetPointsDumb( const Tri& tri ) const{
  std::vector<PathPt> pts( GetPoints( tri ) );
  std::vector<Point> v;
  for ( size_t i = 0; i != m_points.size(); i++ ){
    PathPt& pt( pts[i] );
    v.push_back( Point( pt.x, pt.y ) );
  }
  return v;
}

Tri Points::GetTri() const {
  return m_tri;
}

void Points::SetTri( const Tri& tri ){
  m_tri = tri;
}

void Points::Append( const PathPt& pt ){
  m_points.push_back( pt );
  m_tri = TriFromPoints( m_points );
}

void Points::Append( const Point& pt ){
  m_points.push_back( PathPt( PathPt::LineTo, pt.x, pt.y ) );
  m_tri = TriFromPoints( m_points );
}

void Points::AdjustBack( const PathPt& pt ){
  AdjustBack( Point(pt.x, pt.y ) );
}

void Points::AdjustBack( const Point& pt ){
  PathPt& back( m_points.back() );
  back.x = pt.x;
  back.y = pt.y;
  m_tri = TriFromPoints( m_points );
}

void Points::PopBack(){
  m_points.pop_back();
  m_tri = TriFromPoints( m_points );
}

void Points::Clear(){
  m_points.clear();
  m_tri = Tri( Point(0,0), 0.0, 0, 0 );
}

size_t Points::Size() const{
  return m_points.size();
}
