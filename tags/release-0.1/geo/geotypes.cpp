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

#include "geotypes.hh"
#include <algorithm>
#include "util/angle.hh"
#include "util/util.hh"
using std::min;
using std::max;
using std::abs;

IntPoint truncated( const Point& p ){
  return IntPoint( static_cast<int>(p.x), static_cast<int>(p.y) );
}

Point floated( const IntPoint& p ){
  return Point(static_cast<float>(p.x), static_cast<float>(p.y));
}

Rect floated( const IntRect& r ){
  return Rect( floated(r.TopLeft()), floated(r.GetSize()) );
}

Size floated( const IntSize& sz ){
  return Size( static_cast<float>(sz.w), static_cast<float>(sz.h) );
}


IntSize truncated( const Size& sz ){
  return IntSize( static_cast<int>(sz.w), static_cast<int>(sz.h) );
}

IntRect truncated( const Rect& r ){
  return IntRect( truncated(r.TopLeft() ), truncated( r.GetSize() ) );
}

Rect BoundingRect( const Point& p0, const Point& p1, const Point& p2 ){
  return Rect( MinCoords(p0, p1, p2), MaxCoords( p0,p1 ,p2) );
}

std::vector<Point> Corners( const Rect& r ){
  std::vector<Point> corners;
  corners.push_back( r.TopLeft() );
  corners.push_back( r.TopRight() );
  corners.push_back( r.BottomLeft() );
  corners.push_back( r.BottomRight() );
  return corners;
}
