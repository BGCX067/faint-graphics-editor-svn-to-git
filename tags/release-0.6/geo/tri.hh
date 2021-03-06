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
#include "util/commonfwd.hh"
#include "point.hh"

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
  bool operator==( const Tri& ) const;
private:
  friend Adj get_adj( const Tri&, const Tri& );
  Point m_p0;
  Point m_p1;
  Point m_p2;
};

typedef std::vector<Tri> tris_t;

Rect bounding_rect( const Tri& );
Point center_point( const Tri& );
Adj get_adj( const Tri&, const Tri& );
Point mid_P0_P1( const Tri& );
Point mid_P0_P2( const Tri& );
Point mid_P1_P3( const Tri& );
Point mid_P2_P3( const Tri& );
Tri offset_aligned( const Tri&, faint::coord dX, faint::coord dY );
Tri rotated( const Tri&, faint::radian angle, Point origin );
Tri scaled( const Tri&, const Scale&, const Point& origin );
Tri skewed( const Tri& t, faint::coord skewX );
Tri translated( const Tri&, faint::coord tX, faint::coord tY );
Tri tri_from_rect( const Rect& );

#endif
