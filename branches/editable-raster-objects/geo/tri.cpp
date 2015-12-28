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
#include <cmath>
#include <utility>
#include "geo/geotypes.hh"
#include "geo/pathpt.hh"
#include "geo/tri.hh"
#include "util/angle.hh"
#include "util/util.hh"

using faint::pi;

faint::coord delta_from_length( faint::coord length ){
  return length < 0 ? ( length + 1  ) : ( length - 1);
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
  return distance(m_p0, m_p1);
}

faint::coord Tri::Height() const {
  faint::radian angle1 = rad_angle( m_p0.x, m_p0.y, m_p1.x, m_p1.y );
  faint::radian angle2 = rad_angle( m_p0.x, m_p0.y, m_p2.x, m_p2.y );
  faint::radian angle3 = angle1 + pi / 2.0;
  faint::coord h = distance(m_p0,m_p2);

  if ( std::fabs( std::sin( angle2 ) - std::sin( angle3 ) ) > LITCRD(0.1) ){
    return -h;
  }
  else if ( fabs( std::cos( angle2 ) - std::cos( angle3 ) ) > LITCRD(0.1) ){
    return -h;
  }
  return h;
}

faint::coord Tri::Skew() const{
  Point p0 = rotate_point( m_p0, -Angle(), m_p2 );
  return p0.x - m_p2.x;
}

bool Tri::operator==(const Tri& other ) const{
  return m_p0 == other.m_p0 && m_p1 == other.m_p1 && m_p2 == other.m_p2;
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
  const faint::coord dx = Width();
  const faint::radian angle = Angle();
  return m_p2 + Point( dx * cos( angle ), dx * sin( angle ) );
}

bool Tri::Contains( const Point& pt ) const{
  Rect r( union_of( Rect( m_p0, P1() ), Rect( P1(), P2() ) ) );
  return r.Contains(pt);
}

Rect bounding_rect( const Tri& tri ){
  Point p0 = tri.P0();
  Point p1 = tri.P1();
  Point p2 = tri.P2();
  Point p3 = tri.P3();
  Point minPt = min_coords(p0, p1, p2, p3);
  Point maxPt = max_coords(p0, p1, p2, p3);
  return Rect( minPt, maxPt );
}

Point center_point( const Tri& t ){
  return t.P0() +
    Point( ( t.Width() / 2.0 ) * cos(t.Angle()),
      ( t.Width() / 2.0 ) * sin( t.Angle() ) ) +
    Point( (t.Height()/2.0) * cos( t.Angle() + pi / 2.0 ),
      (t.Height()/2.0) * sin( t.Angle() + pi / 2.0 ) );
}

Adj get_adj( const Tri& t1, const Tri& t2 ){
  Adj adj;

  // De-skew
  Tri t1b = skewed(t1, -t1.Skew());
  Tri t2b = skewed(t2, -t2.Skew());
  adj.scale_x = t1b.Width() == 0 ? 1 :
    t2b.Width() / t1b.Width();
  adj.scale_y = t1b.Height() == 0 ? 1 :
    t2b.Height() / t1b.Height();
  adj.angle = t2b.Angle() - t1b.Angle();
  adj.tr_x = t2b.P3().x - t1b.P3().x;
  adj.tr_y = t2b.P3().y - t1b.P3().y;
  adj.skew = t2.Skew() - t1.Skew();
  return adj;
}

Point mid_P0_P1( const Tri& t ){
  return t.P0() + Point( ( t.Width() / 2.0 ) * cos(t.Angle()),
    ( t.Width() / 2.0 ) * sin(t.Angle()) );

}
Point mid_P0_P2( const Tri& t ){
  return t.P0() + Point( (t.Height())/2.0 * cos( t.Angle() + pi / 2.0 ),
    (t.Height() / 2.0) * sin( t.Angle() + pi / 2.0 ) );
}

Point mid_P1_P3( const Tri& t ){
  return t.P1() + Point( (t.Height()/2.0) * cos( t.Angle() + pi / 2.0 ),
    (t.Height()/2.0) * sin( t.Angle() + pi / 2.0 ) );
}

Point mid_P2_P3( const Tri& t ){
  return t.P2() + Point( (t.Width() / 2.0) * cos( t.Angle() ),
    (t.Width() / 2.0) * sin( t.Angle() ) );
}

Tri offset_aligned( const Tri& t, faint::coord xOff, faint::coord yOff ){
  faint::coord dx =
    xOff * cos( t.Angle() ) +
    yOff * sin( -t.Angle() );

  faint::coord dy =
    xOff * sin( t.Angle() ) +
    yOff * cos( t.Angle() );
  return translated( t, dx, dy );
}

Tri rotated( const Tri& t0, faint::radian angle, Point origin ){
  Point p0 = rotate_point( t0.P0(), angle, origin );
  Point p1 = rotate_point( t0.P1(), angle, origin );
  Point p2 = rotate_point( t0.P2(), angle, origin );
  return Tri( p0, p1, p2 );
}

Tri scaled( const Tri& t0, const Scale& scale, const Point& origin ){
  Point p0 = scale_point( t0.P0(), scale, origin );
  Point p1 = scale_point( t0.P1(), scale, origin );
  Point p2 = scale_point( t0.P2(), scale, origin );
  return Tri( p0, p1, p2 );
}

Tri skewed( const Tri& t, faint::coord skewX ){
  Point p0 = t.P0();
  Point p1 = t.P1();
  Point p2 = t.P2();
  Point p3 = t.P3();

  p0 = rotate_point( p0, -t.Angle(), p2 );
  p1 = rotate_point( p1, -t.Angle(), p3 );
  p0.x += skewX;
  p1.x += skewX;
  p0 = rotate_point( p0, t.Angle(), p2 );
  p1 = rotate_point( p1, t.Angle(), p3 );
  return Tri( p0, p1, p2 );
}

Tri translated( const Tri& t, faint::coord tX, faint::coord tY ){
  Point off( tX, tY );
  return Tri( t.P0() + off, t.P1() + off, t.P2() + off );
}

Tri tri_from_rect( const Rect& r ){
  return Tri( r.TopLeft(), r.TopRight(), r.BottomLeft() );
}

bool valid( const Tri& t ){
  return valid(t.m_p0) &&
    valid(t.m_p1) &&
    valid(t.m_p2);
}

bool rather_zero( const Tri& t ){
  return rather_zero(t.Width()) || rather_zero(t.Height());
}
