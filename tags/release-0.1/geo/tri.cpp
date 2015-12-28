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
#include <utility>
#include "tri.hh"
#include "util/angle.hh"
#include "util/util.hh"

using faint::pi;

faint::coord delta_from_length( faint::coord length ){
  return length < 0 ? ( length + 1  ) : ( length - 1);
}

faint::coord length_from_delta( faint::coord delta ){
  return delta < 0 ? ( delta - 1 ) : ( delta + 1 );
}

Tri::Tri()
  : m_p0(0,0),
    m_p1(0,0),
    m_p2(0,0)
{}

Tri::Tri( const Point& p0, const Point& p1, faint::coord h )
  : m_p0(p0),
    m_p1(p1)
{
  faint::radian angle = rad_angle( p0.x, p0.y, p1.x, p1.y );
  faint::coord dy = delta_from_length( h );
  m_p2 = m_p0 + Point( dy * cos( angle + pi / 2 ), dy * sin( angle + pi / 2 ) );
}

Tri::Tri( const Point& p0, faint::radian angle, faint::coord w, faint::coord h )
  : m_p0( p0 )
{
  faint::coord dx = delta_from_length( w );
  faint::coord dy = delta_from_length( h );
  m_p1 = m_p0 + Point( dx * cos( angle ), dx * sin( angle ) );
  m_p2 = m_p0 + Point( dy * cos( angle + pi / 2 ), dy * sin( angle + pi / 2 ) );
}

Tri::Tri( const Point& p0, const Point& p1, const Point& p2 )
  : m_p0(p0),
    m_p1(p1),
    m_p2(p2)
{}

faint::radian Tri::Angle() const {
  return rad_angle( m_p0.x, m_p0.y, m_p1.x, m_p1.y );
}

faint::coord Tri::Width() const {
  return length_from_delta(distance(m_p0, m_p1));
}

faint::coord Tri::Height() const {
  faint::radian angle1 = rad_angle( m_p0.x, m_p0.y, m_p1.x, m_p1.y );
  faint::radian angle2 = rad_angle( m_p0.x, m_p0.y, m_p2.x, m_p2.y );
  faint::radian angle3 = angle1 + pi / 2.0;
  faint::coord h = length_from_delta(distance(m_p0, m_p2));
  if ( std::fabs( std::sin( angle2 ) - std::sin( angle3 ) ) > LITCRD(0.1) ){
    return -h;
  }
  else if ( fabs( std::cos( angle2 ) - std::cos( angle3 ) ) > LITCRD(0.1) ){
    return -h;
  }
  return h;
}

faint::coord Tri::Skew() const{
  Point p0 = RotatePoint( m_p0, -Angle(), m_p2 );
  return p0.x - m_p2.x;
}

Adj GetAdj( const Tri& t1, const Tri& t2 ){
  Adj adj;

  // De-skew
  Tri t1b = Skewed(t1, -t1.Skew());
  Tri t2b = Skewed(t2, -t2.Skew());
  // Fixme: The width is 0 e.g. for paths that describe vertical lines
  // This hack fixes vertical lines from GraphViz, but this should be solved
  // in some general way
  if ( t1b.Width() == 0 ){
    adj.scale_x = 1;
  }
  else {
    adj.scale_x = t2b.Width() / t1b.Width();
  }
  adj.scale_y = t2b.Height() / t1b.Height();

  adj.angle = t2b.Angle() - t1b.Angle();
  adj.tr_x = t2b.P3().x - t1b.P3().x;
  adj.tr_y = t2b.P3().y - t1b.P3().y;
  adj.skew = t2.Skew() - t1.Skew();
  return adj;
}

Point Tri::P0() const {
  return m_p0;
}

Point Tri::P1() const {
  return m_p1;
}

Point Tri::P2() const {
  return m_p2;
}

Point Tri::P3() const {
  const faint::coord dx = delta_from_length(Width());
  const faint::radian angle = Angle();
  return m_p2 + Point( dx * cos( angle ), dx * sin( angle ) );
}

bool Tri::Contains( const Point& pt ) const{
  Rect r( Union( Rect( m_p0, P1() ), Rect( P1(), P2() ) ) );
  return r.Contains(pt);
}

Tri TriFromRect( const Rect& r ){
  return Tri( r.TopLeft(), r.TopRight(), r.BottomLeft() );
}

Rect BoundingRect( const Tri& tri ){
  Point p0 = tri.P0();
  Point p1 = tri.P1();
  Point p2 = tri.P2();
  Point p3 = tri.P3();
  Point minPt = MinCoords(p0, p1, p2,p3);
  Point maxPt = MaxCoords(p0, p1, p2,p3);  
  return Rect( minPt, maxPt );
}

Point RotatePoint( const Point& pt, faint::radian angle, const Point& origin ){
  Point p2 = pt - origin;
  faint::coord r = distance( p2, Point(0,0) );
  faint::radian alpha = rad_angle( p2.x, p2.y, 0, 0 );
  Point p3( r*cos( alpha + angle + pi ), r*sin( alpha + angle + pi ) );
  return p3 + origin;
}

PathPt RotatePoint( const PathPt& pt, faint::radian angle, const Point& origin ){
  Point p( RotatePoint( Point( pt.x, pt.y ), angle, origin ) );
  Point c( RotatePoint( Point( pt.cx, pt.cy ), angle, origin ) );
  Point d( RotatePoint( Point( pt.dx, pt.dy ), angle, origin ) );
  return PathPt( pt.type, p.x, p.y, c.x, c.y, d.x, d.y  );
}

Point ScalePoint( const Point& pt, faint::coord sx, faint::coord sy, const Point& origin ){
  Point p2 = pt - origin;
  p2.x *= sx;
  p2.y *= sy;
  return p2 + origin;
}

PathPt ScalePoint( const PathPt& pt, faint::coord sx, faint::coord sy, const Point& origin ){
  PathPt p2( pt );
  p2.x = ( p2.x - origin.x ) * sx + origin.x;
  p2.y = ( p2.y - origin.y ) * sy + origin.y;

  p2.cx = ( p2.cx - origin.x ) * sx + origin.x;
  p2.cy = ( p2.cy - origin.y ) * sy + origin.y;

  p2.dx = ( p2.dx - origin.x ) * sx + origin.x;
  p2.dy = ( p2.dy - origin.y ) * sy + origin.y;
  return p2;
}

Tri Rotated( const Tri& t0, faint::radian angle, Point origin ){
  Point p0 = RotatePoint( t0.P0(), angle, origin );
  Point p1 = RotatePoint( t0.P1(), angle, origin );
  Point p2 = RotatePoint( t0.P2(), angle, origin );
  return Tri( p0, p1, p2 );
}

Tri Scaled( const Tri& t0, faint::coord scX, faint::coord scY, Point origin ){
  Point p0 = ScalePoint( t0.P0(), scX, scY, origin );
  Point p1 = ScalePoint( t0.P1(), scX, scY, origin );
  Point p2 = ScalePoint( t0.P2(), scX, scY, origin );
  return Tri( p0, p1, p2 );
}

Tri Skewed( const Tri& t, faint::coord skewX ){
  Point p0 = t.P0();
  Point p1 = t.P1();
  Point p2 = t.P2();
  Point p3 = t.P3();

  p0 = RotatePoint( p0, -t.Angle(), p2 );
  p1 = RotatePoint( p1, -t.Angle(), p3 );
  p0.x += skewX;
  p1.x += skewX;
  p0 = RotatePoint( p0, t.Angle(), p2 );
  p1 = RotatePoint( p1, t.Angle(), p3 );
  return Tri( p0, p1, p2 );
}

Tri Translated( const Tri& t, faint::coord tX, faint::coord tY ){
  Point off( tX, tY );
  return Tri( t.P0() + off, t.P1() + off, t.P2() + off );
}

Tri OffsetAligned( const Tri& t, faint::coord xOff, faint::coord yOff ){
  faint::coord dx =
    xOff * cos( t.Angle() ) +
    yOff * sin( -t.Angle() );

  faint::coord dy =
    xOff * sin( t.Angle() ) +
    yOff * cos( t.Angle() );
  return Translated( t, dx, dy );
}

Point MidP0_P1( const Tri& t ){
  return t.P0() + Point( ( delta_from_length(t.Width()) / 2.0 ) * cos(t.Angle()),
    ( delta_from_length(t.Width()) / 2.0 ) * sin( t.Angle() ) );

}
Point MidP0_P2( const Tri& t ){
  return t.P0() + Point( (delta_from_length(t.Height())/2.0) * cos( t.Angle() + pi / 2.0 ),
    (delta_from_length(t.Height())/2.0) * sin( t.Angle() + pi / 2.0 ) );
}

Point MidP2_P3( const Tri& t ){
  return t.P2() + Point( (delta_from_length(t.Width())/2.0) * cos( t.Angle() ),
    (delta_from_length(t.Width())/2.0) * sin( t.Angle() ) );
}

Point MidP1_P3( const Tri& t ){
  return t.P1() + Point( (delta_from_length(t.Height())/2.0) * cos( t.Angle() + pi / 2.0 ),
    (delta_from_length(t.Height())/2.0) * sin( t.Angle() + pi / 2.0 ) );
}

Point CenterPoint( const Tri& t ){
  return t.P0() +
    Point( ( delta_from_length(t.Width()) / 2.0 ) * cos(t.Angle()),
      ( delta_from_length(t.Width()) / 2.0 ) * sin( t.Angle() ) )  +
    Point( (delta_from_length(t.Height())/2.0) * cos( t.Angle() + pi / 2.0 ),
      (delta_from_length(t.Height())/2.0) * sin( t.Angle() + pi / 2.0 ) );
}

const Tri NullTri( Point(0,0), Point(0,0), Point(0,0) );
