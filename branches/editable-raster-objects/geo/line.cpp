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

#include "line.hh"
#include "point.hh"
#include "util/util.hh" // For point to point distance

Line::Line( faint::coord in_a, faint::coord in_b, faint::coord in_c )
  : a(in_a),
    b(in_b),
    c(in_c)
{}

LineSegment::LineSegment() :
  m_p0(0,0),
  m_p1(0,0)
{}

LineSegment::LineSegment( const Point& p0, const Point& p1 )
  : m_p0(p0),
    m_p1(p1)
{}

Point LineSegment::P0() const{
  return m_p0;
}

Point LineSegment::P1() const{
  return m_p1;
}

faint::coord determinant( const Line& l1, const Line& l2 ){
  return l1.a * l2.b - l2.a*l1.b;
}

bool parallel( const Line& l1, const Line& l2 ){
  return rather_zero(determinant(l1, l2));
}

faint::coord distance( const Point& p, const Line& l ){
  Point p2 = projection(p, l);
  return distance(p, p2);
}

Point intersection( const Line& l1, const Line& l2 ){
  faint::coord det = determinant(l1, l2);
  assert(!rather_zero(det));
  faint::coord x = (l2.b * l1.c - l1.b * l2.c) / det;
  faint::coord y = (l1.a * l2.c - l2.a * l1.c) / det;
  return Point(x,y);
}

Line unbounded(const LineSegment& segment){
  Point p0(segment.P0());
  Point p1(segment.P1());

  faint::coord a = p1.y - p0.y;
  faint::coord b = p0.x - p1.x;
  faint::coord c = a * p1.x + b * p1.y;
  return Line(a,b,c);
}

static Line perpendicular( const Point& p, const Line& l ){
  assert(l.a != 0);
  assert(l.b != 0);

  faint::coord k = -l.b/l.a;
  faint::coord m = p.y + k * p.x;
  return Line(-k, -1, -m);
}

Point projection( const Point& p, const Line& l ){
  if ( l.a == 0 ){
    return Point(p.x, l.c / l.b);
  }
  else if ( l.b == 0 ){
    return Point(l.c / l.a, p.y);
  }

  Line l2 = perpendicular( p, l );
  return intersection(l, l2);
}

Side side( const Point& A, const Point& B, const Point& C ){
  // This will equal zero if the point C is on the line formed by
  // points A and B, and will have a different sign depending on the
  // side. Which side this is depends on the orientation of your (x,y)
  // coordinates, but you can plug test values for A,B and C into this
  // formula to determine whether negative values are to the left or
  // to the right.
  faint::coord value = (B.x - A.x) * (C.y - A.y) - (B.y - A.y) * (C.x - A.x);
  if ( value == 0 ){
    return Side::ON;
  }
  else if ( value < 0 ){
    return Side::A;
  }
  else {
    return Side::B;
  }
}
