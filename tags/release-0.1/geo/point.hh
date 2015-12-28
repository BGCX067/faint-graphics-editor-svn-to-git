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

#ifndef FAINT_POINT
#define FAINT_POINT
#include "basic.hh"

class Point{
public:
  Point();
  Point( faint::coord x, faint::coord y );
  bool operator==( const Point& ) const;
  bool operator!=( const Point& ) const;
  Point operator-() const;

  faint::coord x;
  faint::coord y;
};

Point operator-( const Point&, const Point& );
Point operator+( const Point&, const Point& );
Point operator*( const Point&, const Point& );
Point operator*( const Point&, faint::coord );
Point operator/( const Point&, faint::coord );
Point MinCoords( const Point&, const Point& );
Point MinCoords( const Point&, const Point&, const Point& );
Point MinCoords( const Point&, const Point&, const Point&, const Point& );
Point MaxCoords( const Point&, const Point& );
Point MaxCoords( const Point&, const Point&, const Point& );
Point MaxCoords( const Point&, const Point&, const Point&, const Point& );

#endif
