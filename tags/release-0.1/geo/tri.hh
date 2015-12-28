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

#ifndef FAINT_TRI_HH
#define FAINT_TRI_HH
#include "geotypes.hh"
#include "pathpt.hh"

struct Adj{
  faint::coord scale_x;
  faint::coord scale_y;
  faint::coord tr_x;
  faint::coord tr_y;
  faint::radian angle;
  faint::coord skew;
};

class Tri {
public:
  Tri();
  Tri( const Point& p0, const Point& p1, faint::coord h);
  Tri( const Point& p0, faint::radian a, faint::coord w, faint::coord h );
  Tri( const Point& p0, const Point& p1, const Point& p2 );
  faint::radian Angle() const;
  faint::coord Width() const;
  faint::coord Height() const;
  faint::coord Skew() const;
  Point P0() const;
  Point P1() const;
  Point P2() const;
  Point P3() const;
  bool Contains( const Point& pt ) const;
private:
  friend Adj GetAdj( const Tri&, const Tri& );
  Point m_p0;
  Point m_p1;
  Point m_p2;
};

Adj GetAdj( const Tri&, const Tri& );
Tri TriFromRect( const Rect& );
Tri Rotated( const Tri&, faint::radian angle, Point origin );
Tri Scaled( const Tri&, faint::coord scX, faint::coord scY, Point origin );
Tri Translated( const Tri&, faint::coord tX, faint::coord tY );
Tri OffsetAligned( const Tri&, faint::coord dX, faint::coord dY );
Rect BoundingRect( const Tri& tri );

Point MidP0_P1( const Tri& );
Point MidP0_P2( const Tri& );
Point MidP2_P3( const Tri& );
Point MidP1_P3( const Tri& );
Point CenterPoint( const Tri& );

Point RotatePoint( const Point& pt, faint::radian angle, const Point& origin );
Point ScalePoint( const Point& pt, faint::coord sx, faint::coord sy, const Point& origin );
PathPt RotatePoint( const PathPt& pt, faint::radian angle, const Point& origin );
PathPt ScalePoint( const PathPt& pt, faint::coord sx, faint::coord sy, const Point& origin );
Tri Skewed( const Tri& t, faint::coord skewX );

void PrintTri( const Tri& );
void PrintAdj( const Adj& );

extern const Tri NullTri;
#endif
