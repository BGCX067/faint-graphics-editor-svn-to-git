// -*- coding: us-ascii-unix -*-
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

#include <cassert>
#include "geo/geo-func.hh"
#include "geo/line.hh"
#include "geo/measure.hh"

namespace faint{

Line::Line(coord in_a, coord in_b, coord in_c)
  : a(in_a),
    b(in_b),
    c(in_c)
{}

LineSegment::LineSegment() :
  p0(0,0),
  p1(0,0)
{}

LineSegment::LineSegment(const Point& in_p0, const Point& in_p1)
  : p0(in_p0),
    p1(in_p1)
{}

LineSegment::LineSegment(const std::pair<Point,Point>& points)
  : LineSegment(points.first, points.second)
{}

std::pair<Point, Point> to_pair(const LineSegment& segment){
  return std::make_pair(segment.p0, segment.p1);
}

IntLineSegment::IntLineSegment(const IntPoint& in_p0, const IntPoint& in_p1)
  : p0(in_p0),
    p1(in_p1)
{}

IntLineSegment::IntLineSegment(const std::pair<IntPoint,IntPoint>& pair)
  : IntLineSegment(pair.first, pair.second)
{}

coord length(const LineSegment& l){
  return distance(l.p0, l.p1);
}

coord length(const IntLineSegment& line){
  // Fixme: Duplicates util.cpp
  return length(floated(line));
}

LineSegment operator*(const LineSegment& l, coord scale){
  return {l.p0 * scale, l.p1 * scale};
}

LineSegment operator*(coord scale, const LineSegment& l){
  return l * scale;
}

bool operator==(const LineSegment& lhs, const LineSegment& rhs){
  return lhs.p0 == rhs.p0 && lhs.p1 == rhs.p1;
}

static coord determinant(const Line& l1, const Line& l2){
  return l1.a * l2.b - l2.a*l1.b;
}

coord distance(const Point& p, const Line& l){
  Point p2 = projection(p, l);
  return distance(p, p2);
}

LineSegment floated(const IntLineSegment& l){
  return LineSegment(floated(l.p0), floated(l.p1));
}

Point intersection(const Line& l1, const Line& l2){
  coord det = determinant(l1, l2);
  assert(!rather_zero(det)); // Lines are parallel
  coord x = (l2.b * l1.c - l1.b * l2.c) / det;
  coord y = (l1.a * l2.c - l2.a * l1.c) / det;
  return Point(x,y);
}

Line unbounded(const LineSegment& seg){
  coord a = seg.p1.y - seg.p0.y;
  coord b = seg.p0.x - seg.p1.x;
  coord c = a * seg.p1.x + b * seg.p1.y;
  return Line(a,b,c);
}

static Line perpendicular(const Point& p, const Line& l){
  assert(l.a != 0);
  assert(l.b != 0);

  coord k = -l.b/l.a;
  coord m = p.y + k * p.x;
  return Line(-k, -1, -m);
}

Point projection(const Point& p, const Line& l){
  if (l.a == 0){
    return Point(p.x, l.c / l.b);
  }
  else if (l.b == 0){
    return Point(l.c / l.a, p.y);
  }

  Line l2 = perpendicular(p, l);
  return intersection(l, l2);
}

Side side(const Point& p, const LineSegment& l){
  const Point& A = l.p0;
  const Point& C = l.p1;
  coord value = (p.x - A.x) * (C.y - A.y) - (p.y - A.y) * (C.x - A.x);
  if (value == 0){
    return Side::ON;
  }
  else if (value < 0){
    return Side::A;
  }
  else {
    return Side::B;
  }
}

LineSegment translated(const LineSegment& l, const Point& tr){
  return LineSegment(l.p0 + tr, l.p1 + tr);
}

} // namespace
