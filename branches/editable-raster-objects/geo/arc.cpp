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
#include <iostream>
#include "geo/arc.hh"
#include "geo/tri.hh"
#include "util/angle.hh"

AngleSpan::AngleSpan()
  : start(0.0),
    stop(0.0)
{}

AngleSpan::AngleSpan( faint::radian in_start, faint::radian in_stop )
  : start(in_start),
    stop(in_stop)
{}

using faint::pi;

int required_curve_count( const AngleSpan& angles ){
  faint::radian span = fabs( angles.start - angles.stop );
  if ( span <= pi / 2 ){
    return 2;
  }
  else if ( span <= pi ) {
    return 4;
  }
  else if ( span <= 3 * pi / 2 ){
    return 6;
  }
  return 8;
}

ArcEndPoints::ArcEndPoints( const Tri& tri, const AngleSpan& angles ){
  faint::coord rx = tri.Width() / 2;
  faint::coord ry = tri.Height() / 2;
  faint::coord aCosEta1 = rx * cos(angles.start);
  faint::coord bSinEta1 = ry * sin(angles.start);
  Point c(center_point(tri));
  faint::radian mainAngle(tri.Angle());
  faint::coord x1 = c.x + aCosEta1 * cos(mainAngle) - bSinEta1 * sin(mainAngle);
  faint::coord y1 = c.y + aCosEta1 * sin(mainAngle) + bSinEta1 * cos(mainAngle);

  // end point
  double aCosEta2 = rx * cos(angles.stop);
  double bSinEta2 = ry * sin(angles.stop);
  faint::coord x2 = c.x + aCosEta2 * cos(mainAngle) - bSinEta2 * sin(mainAngle);
  faint::coord y2 = c.y + aCosEta2 * sin(mainAngle) + bSinEta2 * cos(mainAngle);

  m_p0 = Point(x1, y1);
  m_p1 = Point(x2, y2);
}

Point ArcEndPoints::operator[](size_t i) const{
  assert(i <= 1);
  return i == 0 ? m_p0 : m_p1;
}

ArcEndPoints::operator std::vector<Point>() const{
  std::vector<Point> pts;
  pts.push_back(m_p0);
  pts.push_back(m_p1);
  return pts;
}

std::vector<Point> arc_as_path( const Tri& tri, const AngleSpan& angles ){
  // This algorithm is based on documentation and code from SpaceRoots.org, by L. Maisonobe
  // "Drawing an elliptical arc using polylines, quadratic or cubic Bezier curves" and
  // "EllipticalArc.java"
  // Available here: http://www.spaceroots.org/downloads.html

  const int numCurves = required_curve_count( angles );
  const double rx = tri.Width() / 2.0;
  const double ry = tri.Height() / 2.0;
  const faint::radian mainAngle(tri.Angle());
  const Point c(center_point(tri));

  faint::radian curveSpan = ( angles.stop - angles.start ) / numCurves;
  faint::radian currentAngle = angles.start;
  faint::coord cosCurrentAngle = cos(currentAngle);
  faint::coord sinCurrentAngle = sin(currentAngle);
  faint::coord rxCosCurrentAngle = rx * cosCurrentAngle;
  faint::coord rySinCurrentAngle = ry * sinCurrentAngle;
  faint::coord rxSinCurrentAngle = rx * sinCurrentAngle;
  faint::coord ryCosCurrentAngle = ry * cosCurrentAngle;
  faint::coord sinMainAngle = sin(mainAngle);
  faint::coord cosMainAngle = cos(mainAngle);

  faint::coord x = c.x + rxCosCurrentAngle * cosMainAngle - rySinCurrentAngle * sinMainAngle;
  faint::coord y = c.y + rxCosCurrentAngle * sinMainAngle + rySinCurrentAngle * cosMainAngle;
  faint::coord xDot = -rxSinCurrentAngle * cosMainAngle - ryCosCurrentAngle * sinMainAngle;
  faint::coord yDot = -rxSinCurrentAngle * sinMainAngle + ryCosCurrentAngle * cosMainAngle;

  // Fixme: Using Point requires knowledge that one should move_to or line_to v[0],
  // and then use curve_to with the remaining points (in sets of three),
  // use PathPt (or similar) instead.
  std::vector<Point> v;
  // Arc start point
  v.push_back(Point(x, y));
  faint::coord t = tan(0.5 * curveSpan);
  faint::coord alpha = sin(curveSpan) * (sqrt(4 + 3 * t * t) - 1) / 3.0;

  for ( int i = 0; i != numCurves; i++ ){
    faint::coord prevX = x;
    faint::coord prevY = y;
    double prevXDot = xDot;
    double prevYDot = yDot;
    currentAngle += curveSpan;
    cosCurrentAngle = cos(currentAngle);
    sinCurrentAngle = sin(currentAngle);
    rxCosCurrentAngle = rx * cosCurrentAngle;
    rySinCurrentAngle = ry * sinCurrentAngle;
    rxSinCurrentAngle = rx * sinCurrentAngle;
    ryCosCurrentAngle = ry * cosCurrentAngle;
    x = c.x + rxCosCurrentAngle * cosMainAngle - rySinCurrentAngle * sinMainAngle;
    y = c.y + rxCosCurrentAngle * sinMainAngle + rySinCurrentAngle * cosMainAngle;
    xDot = -rxSinCurrentAngle * cosMainAngle - ryCosCurrentAngle * sinMainAngle;
    yDot = -rxSinCurrentAngle * sinMainAngle + ryCosCurrentAngle * cosMainAngle;
    v.push_back(Point(prevX + alpha * prevXDot, prevY + alpha * prevYDot));
    v.push_back(Point(x - alpha * xDot, y - alpha * yDot));
    v.push_back(Point(x, y));
  }
  return v;
}
