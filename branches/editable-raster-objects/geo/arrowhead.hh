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

#ifndef FAINT_ARROWHEAD
#define FAINT_ARROWHEAD
#include "geo/point.hh"
class LineSegment;
class Rect;
class ArrowHead{
public:
  ArrowHead( const Point&, const Point&, const Point& );
  Rect BoundingBox() const;
  Point LineAnchor() const;
  Point P0() const;
  Point P1() const;
  Point P2() const;
private:
  Point m_p0;
  Point m_p1;
  Point m_p2;
};

ArrowHead get_arrowhead( const LineSegment&, faint::coord lineWidth );

#endif
