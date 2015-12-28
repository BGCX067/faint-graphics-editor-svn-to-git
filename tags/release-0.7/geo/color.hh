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

#ifndef FAINT_COLOR
#define FAINT_COLOR
#include "util/commonfwd.hh"

namespace faint{
  class Color{
  public:
    Color();
    Color( uchar r, uchar g, uchar b, uchar a = 255 );
    bool operator==( const Color& ) const;
    bool operator!=( const Color& ) const;

    // Simple field-by-field less for containers.
    bool operator<( const Color& c ) const;
    uchar r;
    uchar g;
    uchar b;
    uchar a;
  };

  // Creates a Color object from int values. Truncates the values to
  // the allowed range
  Color color_from_ints( int r, int g, int b, int a=255);
  Color color_from_uints( uint r, uint g, uint b, uint a=255);
  bool opaque( const Color& );
  bool translucent( const Color& );
  bool valid_color( int r, int g, int b, int a );
  Color white_color();
}

#endif
