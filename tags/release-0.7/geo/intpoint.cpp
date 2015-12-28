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
#include "point.hh"

IntPoint::IntPoint() :
  x(0),
  y(0)
{}

IntPoint::IntPoint( int in_x, int in_y ) :
  x(in_x),
  y(in_y)
{}

IntPoint::IntPoint( size_t in_x, size_t in_y ) :
  x(in_x),
  y(in_y)
{}

bool IntPoint::operator==( const IntPoint& other ) const{
  return x == other.x && y == other.y;
}

bool IntPoint::operator!=( const IntPoint& other ) const{
  return !operator==(other);
}

IntPoint IntPoint::operator-() const {
  return IntPoint( -x, -y );
}

bool IntPoint::operator<( const IntPoint& other ) const{
  return y < other.y || (y == other.y && x < other.x );
}


IntPoint operator-( const IntPoint& lhs, const IntPoint& rhs ){
  return IntPoint( lhs.x - rhs.x, lhs.y - rhs.y );
}

IntPoint operator-( const IntPoint& p, int delta ){
  return IntPoint( p.x - delta, p.y - delta );
}

IntPoint operator+( const IntPoint& p, int delta ){
  return IntPoint( p.x + delta, p.y + delta );
}

IntPoint operator+( const IntPoint& lhs, const IntPoint& rhs ){
  return IntPoint(lhs.x + rhs.x, lhs.y + rhs.y);
}

Point operator*( const IntPoint& p, faint::coord scale){
  return Point(p.x * scale, p.y * scale);
}

Point operator*( faint::coord scale, const IntPoint& p){
  return operator*(p, scale);
}
