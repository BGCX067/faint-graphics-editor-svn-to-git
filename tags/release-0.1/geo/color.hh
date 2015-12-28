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

namespace faint{
  typedef unsigned char uchar;
  class Color{
  public:
    Color();
    Color( uchar r, uchar g, uchar b, uchar a = 255 );    
    bool operator==( const Color& c2 ) const;
    bool operator!=( const Color& c2 ) const;
    uchar r;
    uchar g;
    uchar b;
    uchar a;
  };

  bool valid_color( int r, int g, int b, int a );
  Color ColorFromInts( int r, int g, int b, int a );
}



#endif
