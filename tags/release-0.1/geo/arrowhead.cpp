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
#include "basic.hh"

#include "size.hh"
#include "line.hh"
#include "rect.hh"
#include "arrowhead.hh"
#include "angle.hh"
#include "util.hh"
ArrowHead::ArrowHead( const Point& p0, const Point& p1, const Point& p2 )
  : m_p0(p0),
    m_p1(p1),
    m_p2(p2)
{}

Point ArrowHead::P0() const{
  return m_p0;
}

Point ArrowHead::P1() const{
  return m_p1;
}

Point ArrowHead::P2() const{
  return m_p2;
}

ArrowHead GetArrowHead( const Line& l, faint::coord lineWidth ){
  faint::coord x0( l.P0().x );
  faint::coord x1( l.P1().x );
  faint::coord y0( l.P0().y );
  faint::coord y1( l.P1().y );

  faint::radian angle = rad_angle( x1, y1, x0, y0);
  faint::coord orth = angle + faint::pi / 2;
  faint::coord ax0 = cos( orth ) * 10 * ( lineWidth / 3.0f );
  faint::coord ay0 = sin( orth ) * 10 * ( lineWidth / 3.0f );

  faint::coord ax1 = cos( angle ) * 15 * ( lineWidth / 2.0f );
  faint::coord ay1 = sin( angle ) * 15 * ( lineWidth / 2.0f );
  Point p1( x1 + ax0 + ax1, y1 + ay0 + ay1 );
  Point p2( x1, y1 );
  Point p3( x1 - ax0 + ax1, y1 - ay0 + ay1 );
  return ArrowHead( p1, p2, p3 );
}

Point ArrowHead::LineAnchor() const{
    return MidPoint(m_p0, m_p2);
}

Rect ArrowHead::BoundingBox() const{
  Point p0(MinCoords(m_p0, m_p1, m_p2));
  Point p1(MaxCoords(m_p0, m_p1, m_p2));  
  return Rect( p0, p1 );
}
