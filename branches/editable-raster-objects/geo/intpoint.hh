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

#ifndef FAINT_INTPOINT
#define FAINT_INTPOINT
#include "geo/basic.hh"
#include <cstdlib>

class IntPoint{
public:
  IntPoint();
  IntPoint( int x, int y );

  void operator+=( const IntPoint& );
  void operator-=( const IntPoint& );
  bool operator==( const IntPoint& ) const;
  bool operator!=( const IntPoint& ) const;
  IntPoint operator-() const;
  bool operator<( const IntPoint& ) const;
  int x;
  int y;
private:
  IntPoint( faint::coord, faint::coord );
  IntPoint( float, float );
  IntPoint( size_t, size_t );

};

bool fully_positive( const IntPoint& );
IntPoint max_coords( const IntPoint&, const IntPoint& );
IntPoint min_coords( const IntPoint&, const IntPoint& );
IntPoint operator-( const IntPoint&, const IntPoint& );
IntPoint operator-( const IntPoint&, int );
IntPoint operator+( const IntPoint&, int );
IntPoint operator+( const IntPoint&, const IntPoint& );
IntPoint operator*( const IntPoint&, const IntPoint& );
IntPoint operator/( const IntPoint&, const IntPoint& );
class Point;
Point operator*( const IntPoint&, faint::coord scale );
Point operator*( faint::coord scale, const IntPoint& );

#endif
