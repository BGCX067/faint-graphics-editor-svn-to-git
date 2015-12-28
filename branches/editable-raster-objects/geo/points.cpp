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

Tri tri_from_points( const std::vector<PathPt>& points ){
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
    if ( PathPt::Close == pt.type ){
      break;
    }
    x_min = std::min( x_min, pt.x );
    x_max = std::max( x_max, pt.x );
    y_min = std::min( y_min, pt.y );
    y_max = std::max( y_max, pt.y );
  }
  return Tri( Point( x_min, y_min ), Point(x_max, y_min), Point(x_min, y_max) );
}

Tri tri_from_points( const std::vector<Point>& points ){
  if ( points.empty() ){
    return Tri( Point(0,0), 0, 0, 0 );
  }
  const Point& p0 = points[0];
  faint::coord x_min = p0.x;
  faint::coord x_max = p0.x;
  faint::coord y_min = p0.y;
  faint::coord y_max = p0.y;

  for ( size_t i = 0; i != points.size(); i++ ){
    const Point& pt = points[i];
    x_min = std::min( x_min, pt.x );
    x_max = std::max( x_max, pt.x );
    y_min = std::min( y_min, pt.y );
    y_max = std::max( y_max, pt.y );
  }
  return Tri( Point( x_min, y_min ), Point(x_max, y_min), Point(x_min, y_max) );
}

Points::Points()
  : m_tri( Point(0,0), 0, 0, 0 ),
    m_cacheTri(m_tri)
{}

Points::Points( const Points& other )
  : m_tri( other.m_tri ),
    m_points( other.m_points ),
    m_cache(),
    m_cacheTri()
{}

Points::Points( const std::vector<PathPt>& points ) :
  m_tri( tri_from_points( points ) ),
  m_points(points),
  m_cache(),
  m_cacheTri()
{}

Points::Points( const std::vector<Point>& points ){
  for ( size_t i = 0; i != points.size(); i++ ){
    const Point& p( points[i] );
    m_points.push_back( PathPt( PathPt::LineTo, p.x, p.y ) );
  }
  m_tri = tri_from_points( m_points );
}

void Points::AdjustBack( const PathPt& pt ){
  AdjustBack( Point(pt.x, pt.y ) );
}

void Points::AdjustBack( const Point& pt ){
  PathPt& back( m_points.back() );
  back.x = pt.x;
  back.y = pt.y;
  m_tri = tri_from_points( m_points );
}

void Points::Append( const PathPt& pt ){
  m_points.push_back( pt );
  m_tri = tri_from_points( m_points );
}

void Points::Append( const Point& pt ){
  m_points.push_back( PathPt( PathPt::LineTo, pt.x, pt.y ) );
  m_tri = tri_from_points( m_points );
}

void Points::Clear(){
  m_points.clear();
  m_tri = Tri( Point(0,0), 0.0, 0, 0 );
}

std::vector<PathPt> Points::GetPoints( const Tri& tri ) const{
  if ( m_cache.size() == m_points.size() && m_cacheTri == tri ){
    return m_cache;
  }

  if ( m_cache.size() != m_points.size() ){
    m_cache = m_points;
  }
  Adj a = get_adj( m_tri, tri );
  faint::coord p2y = m_tri.P2().y;
  faint::coord ht = m_tri.Height();
  for ( size_t i = 0; i != m_points.size(); i++ ){
    PathPt pt = m_points[i];
    if ( ht != 0 ){
      pt.x += a.skew * ( ( p2y - pt.y ) / ht ) / a.scale_x; // assumes m_tri not rotated
    }
    pt.x += a.tr_x;
    pt.y += a.tr_y;

    if ( ht != 0 ){
      pt.cx += a.skew * ( ( p2y - pt.cy ) / ht ) / a.scale_x; // assumes m_tri not rotated
    }
    pt.cx += a.tr_x;
    pt.cy += a.tr_y;

    if ( ht != 0 ){
      pt.dx += a.skew * ( ( p2y - pt.dy ) / ht ) / a.scale_x; // assumes m_tri not rotated
    }
    pt.dx += a.tr_x;
    pt.dy += a.tr_y;

    pt = scale_point( pt, Scale(a.scale_x, a.scale_y), tri.P3() );
    pt = rotate_point( pt, tri.Angle(), tri.P3() );

    m_cache[i] = pt;
  }
  return m_cache;
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

void Points::InsertPoint( const Tri& tri, const Point& pt, size_t index ){
  m_points = GetPoints( tri );
  m_points.insert(m_points.begin() + index, PathPt::Line(pt.x, pt.y));
  m_tri = tri_from_points(m_points);
}

PathPt Points::PopBack(){
  PathPt p = m_points.back();
  m_points.pop_back();
  m_tri = tri_from_points( m_points );
  return p;
}

void Points::RemovePoint( const Tri& tri, size_t index ){
  m_points = GetPoints( tri );
  m_points.erase( m_points.begin() + index );
  m_tri = tri_from_points(m_points);
}

void Points::SetPoint( const Tri& tri, const Point& inPt, size_t index ){
  m_points = GetPoints( tri );
  m_points[index] = PathPt::Line(inPt.x, inPt.y);
  m_tri = tri_from_points(m_points);
}

void Points::SetTri( const Tri& tri ){
  m_tri = tri;
}

size_t Points::Size() const{
  return m_points.size();
}
