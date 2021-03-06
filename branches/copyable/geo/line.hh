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

#ifndef FAINT_LINE_HH
#define FAINT_LINE_HH
#include <utility>
#include "geo/intpoint.hh"
#include "geo/point.hh"

namespace faint{

class Line{
public:
  // ax + by = c
  Line(coord a, coord b, coord c);
  coord a;
  coord b;
  coord c;
};

class LineSegment{
public:
  LineSegment();
  LineSegment(const Point&, const Point&);
  LineSegment(const std::pair<Point,Point>&);
  Point p0;
  Point p1;
};

std::pair<Point, Point> to_pair(const LineSegment&);
LineSegment operator*(const LineSegment&, coord);
LineSegment operator*(coord, const LineSegment&);
bool operator==(const LineSegment&, const LineSegment&);

class IntLineSegment{
public:
  IntLineSegment(const IntPoint&, const IntPoint&);
  IntLineSegment(const std::pair<IntPoint,IntPoint>&);
  IntPoint p0;
  IntPoint p1;
};


LineSegment floated(const IntLineSegment&);
coord length(const LineSegment&);
coord length(const IntLineSegment&);
LineSegment translated(const LineSegment&, const Point&);

coord distance(const Point&, const Line&);
Point intersection(const Line&, const Line&);
Point projection(const Point&, const Line&);

enum class Side{A, B, ON};
Side side(const Point&, const LineSegment&);

Line unbounded(const LineSegment&);

} // namespace

#endif
