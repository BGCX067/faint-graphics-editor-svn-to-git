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

#include <algorithm> // min, max
#include "color.hh"

unsigned int to_uint( const faint::Color& c ){
  using faint::uint;
  return (uint(c.r)) | (uint(c.g) << 8) | (uint(c.b) << 16) | (uint(c.a) << 24);
}

namespace faint {
  Color::Color()
    : r(0),
      g(0),
      b(0),
      a(255)
  {}

  Color::Color( uchar in_r, uchar in_g, uchar in_b, uchar in_a  )
    : r(in_r),
      g(in_g),
      b(in_b),
      a(in_a)
  {}

  bool Color::operator==( const Color& c2 ) const{
    return r == c2.r && g == c2.g && b == c2.b && a == c2.a;
  }
  bool Color::operator!=( const Color& c2 ) const{
    return r != c2.r || g != c2.g || b != c2.b || a != c2.a;
  }

  bool Color::operator<( const Color& c2 ) const{
    return to_uint(*this) < to_uint(c2);
  }

  int constrained( int lower, int value, int upper ){
    return std::min(std::max(lower, value), upper);
  }

  Color color_from_ints( int ir, int ig, int ib, int ia ){
    faint::uchar r = static_cast<faint::uchar>(constrained(0,ir,255));
    faint::uchar g = static_cast<faint::uchar>(constrained(0,ig,255));
    faint::uchar b = static_cast<faint::uchar>(constrained(0,ib,255));
    faint::uchar a = static_cast<faint::uchar>(constrained(0,ia,255));
    return Color(r,g,b,a);
  }

  bool opaque( const Color& c ){
    return c.a == 255;
  }

  bool translucent( const Color& c ){
    return c.a != 255;
  }

  bool valid_color( int r, int g, int b, int a ){
    return 0 <= r && r <= 255 &&
      0 <= g && g <= 255 &&
      0 <= b && b <= 255 &&
      0 <= a && a <= 255;
  }

  Color white_color(){
    return faint::Color(255,255,255);
  }



} // namespace faint
