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

#include "point.hh"
#include "rect.hh"
#include "size.hh"
#include <algorithm>
using std::min;
using std::max;

Rect::Rect(){
  x = y = w = h = LITCRD(0.0);
}

Rect::Rect( const Point& p1, const Point& p2 ){
  x = min( p1.x, p2.x );
  y = min( p1.y, p2.y );
  w = fabs( p1.x - p2.x ) + LITCRD(1.0);
  h = fabs( p1.y - p2.y ) + LITCRD(1.0);
}

Rect::Rect( const Point& p, const Size& sz ){
  x = p.x;
  y = p.y;
  w = sz.w;
  h = sz.h;
}

Size Rect::GetSize() const{
  return Size( w, h );
}

faint::coord Rect::Left() const {
  return x;
}

faint::coord Rect::Right() const {
  return x + w - LITCRD(1.0);
}

faint::coord Rect::Top() const {
  return y;
}

faint::coord Rect::Bottom() const {
  return y + h - LITCRD(1.0);
}

Point Rect::TopLeft() const {
  return Point( x, y );
}

Point Rect::TopRight() const {
  return Point( Right(), y );
}

Point Rect::BottomLeft() const{
  return Point( x, Bottom() );
}

Point Rect::BottomRight() const {
  return Point( Right(), Bottom() );
}

Point Rect::Center() const{
  return Point( x + (w - LITCRD(1.0)) / LITCRD(2.0), y + (h - LITCRD(1.0)) / LITCRD(2.0) );
}

bool Rect::Contains( const Point& pt ) const{
  return pt.x >= x && pt.y >= y && pt.x <= Right() && pt.y <= Bottom();
}

// Non-member functions

bool Intersects( const Rect& r1, const Rect& r2 ){
  faint::coord x1 = r1.x < r2.x ? r2.x : r1.x;
  faint::coord y1 = r1.y < r2.y ? r2.y : r1.y;
  faint::coord x2 = r1.Right();
  faint::coord y2 = r1.Bottom();

  if ( x2 > r2.Right() ){
    x2 = r2.Right();
  }
  if ( y2 > r2.Bottom() ){
    y2 = r2.Bottom();
  }

  faint::coord w2 = x2 - x1 + LITCRD(1.0);
  faint::coord h2 = y2 - y1 + LITCRD(1.0);
  if ( w2 < 0 || h2 < 0 ){
    return false;
  }
  return true;
}

bool Empty( const Rect& r ){
  return r.w == 0 || r.h == 0;
}

Rect Translated( const Rect& r, const Point& p ){
  return Rect(r.TopLeft() + p, r.GetSize() );
}

Rect Inflated( const Rect& r, faint::coord d ){
  return Inflated( r, d, d );
}

Rect Inflated( const Rect& r, faint::coord dx, faint::coord dy ){
  return Rect( Point(r.x - dx, r.y - dy), Size(r.w + 2 * dx, r.h + 2 * dy) );
}

Rect Union( const Rect& r1, const Rect& r2 ) {
  const faint::coord left = min( r1.x, r2.x );
  const faint::coord top = min( r1.y, r2.y );
  const faint::coord bottom = max( r1.Bottom(), r2.Bottom() );
  const faint::coord right = max( r1.Right(), r2.Right() );
  return Rect( Point(left, top), Point( right, bottom ) );
}

Rect Intersection( const Rect& r1, const Rect& r2 ){
  faint::coord x1 = r1.x < r2.x ? r2.x : r1.x;
  faint::coord y1 = r1.y < r2.y ? r2.y : r1.y;
  faint::coord x2 = r1.Right();
  faint::coord y2 = r1.Bottom();

  if ( x2 > r2.Right() ){
    x2 = r2.Right();
  }
  if ( y2 > r2.Bottom() ){
    y2 = r2.Bottom();
  }

  faint::coord w2 = x2 - x1 + LITCRD(1.0);
  faint::coord h2 = y2 - y1 + LITCRD(1.0);
  if ( w2 <= 0 || h2 <= 0 ){
    return Rect();
  }
  return Rect( Point(x1, y1), Size(w2, h2) );
}
