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

#include "intpoint.hh"
#include "intsize.hh"
#include "intrect.hh"
#include <algorithm>
using std::min;
using std::max;

IntRect::IntRect(){
  x = y = w = h = 0;
}

IntRect::IntRect( const IntPoint& p1, const IntPoint& p2 ) :
  x( min(p1.x, p2.x) ),
  y( min(p1.y, p2.y) ),
  w( abs(p1.x - p2.x) + 1 ),
  h( abs(p1.y - p2.y) + 1 )
{}

IntRect::IntRect( const IntPoint& pt, const IntSize& sz )
  : x(pt.x),
    y(pt.y),
    w(sz.w),
    h(sz.h)
{}

IntSize IntRect::GetSize() const{
  return IntSize( w, h );
}

int IntRect::Left() const{
  return x;
}

int IntRect::Right() const{
  return x + w - 1;
}

int IntRect::Top() const{
  return y;
}

int IntRect::Bottom() const{
  return y + h - 1;
}

IntPoint IntRect::TopLeft() const{
  return IntPoint(x,y);
}

IntPoint IntRect::TopRight() const{
  return IntPoint( Right(),y );
}

IntPoint IntRect::BottomLeft() const{
  return IntPoint( x, Bottom() );
}

IntPoint IntRect::BottomRight() const{
  return IntPoint( Right(), Bottom() );
}

bool IntRect::Contains( const IntPoint& p ) const{
  return p.x >= x && p.y >= y &&
    p.x <= Right() && p.y <= Bottom();
}

void IntRect::MoveTo( const IntPoint& p ){
  x = p.x;
  y = p.y;
}

bool Empty( const IntRect& r ){
  return r.w == 0 || r.h == 0;
}

IntRect Translated( const IntRect& r, const IntPoint& p ){
  return IntRect( IntPoint(r.x + p.x, r.y + p.y), IntSize(r.w, r.h) );
}

IntRect Inflated( const IntRect& r, int dx, int dy ){
  return IntRect( IntPoint(r.x - dx, r.y - dy), IntSize( r.w + 2 * dx, r.h + 2 * dy ) );
}

IntRect Inflated( const IntRect& r, int d ){
  return Inflated( r, d, d );
}

IntRect Union( const IntRect& r1, const IntRect& r2 ) {
  const int left = min( r1.x, r2.x );
  const int top = min( r1.y, r2.y );
  const int bottom = max( r1.Bottom(), r2.Bottom() );
  const int right = max( r1.Right(), r2.Right() );
  return IntRect( IntPoint(left, top), IntSize(right - left + 1, bottom - top + 1 ) );
}

IntRect Intersection( const IntRect& r1, const IntRect& r2 ) {
  int x1 = r1.x < r2.x ? r2.x : r1.x;
  int y1 = r1.y < r2.y ? r2.y : r1.y;
  int x2 = r1.Right();
  int y2 = r1.Bottom();

  if ( x2 > r2.Right() ){
    x2 = r2.Right();
  }
  if ( y2 > r2.Bottom() ){
    y2 = r2.Bottom();
  }

  int w2 = x2 - x1 + 1;
  int h2 = y2 - y1 + 1;
  if ( w2 <= 0 || h2 <= 0 ){
    return IntRect( IntPoint(0, 0), IntSize(0, 0) );
  }
  return IntRect( IntPoint(x1, y1), IntSize(w2, h2) );
}






