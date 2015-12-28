// -*- coding: us-ascii-unix -*-
// Copyright 2013 Lukas Kemmer
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

#include <cmath>
#include "geo/angle.hh"
#include "geo/geo-func.hh"
#include "geo/intrect.hh"
#include "geo/line.hh"
#include "geo/measure.hh"
#include "geo/point.hh"
#include "geo/rect.hh"

namespace faint{

IntRect bounding_rect(const IntPoint& p0, const IntPoint& p1){
  return IntRect(p0, p1);
}

IntRect bounding_rect(const IntPoint& p0, const IntPoint& p1, const IntPoint& p2){
  return IntRect(min_coords(p0, p1, p2), max_coords(p0, p1, p2));
}

Rect bounding_rect(const LineSegment& line){
  return bounding_rect(line.p0, line.p1);
}

IntRect bounding_rect(const IntLineSegment& line){
  return IntRect(line.p0, line.p1);
}

Rect bounding_rect(const Point& p0, const Point& p1){
  return Rect(p0, p1);
}

Rect bounding_rect(const Point& p0, const Point& p1, const Point& p2){
  return Rect(min_coords(p0, p1, p2), max_coords(p0,p1, p2));
}

coord distance(const Point& p0, const Point& p1){
  return sqrt((p0.x - p1.x) * (p0.x - p1.x) +
    (p0.y - p1.y) * (p0.y - p1.y));
}

coord distance(const IntPoint& pt1, const IntPoint& pt2){
  return distance(floated(pt1), floated(pt2));
}

Angle line_angle(const LineSegment& l){
  return atan2(l.p1.y - l.p0.y, l.p1.x - l.p0.x);
}

Angle angle360(const LineSegment& l){
  if (l.p0 == l.p1){
    return Angle::Zero();
  }

  Angle angle = line_angle(l);
  if (angle < Angle::Zero()){
    angle = 2 * pi + angle;
  }
  if (angle == Angle::Zero()){
    return angle;
  }
  return 2 * pi - angle;
}

} // namespace
