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
#include <cmath> // fabs
#include "color.hh"

unsigned int to_uint( const faint::Color& c ){
  using faint::uint;
  return (uint(c.r)) | (uint(c.g) << 8) | (uint(c.b) << 16) | (uint(c.a) << 24);
}

namespace my{
  int min( int a, int b, int c){
    return std::min(std::min(a, b),c);
  }
  int max( int a, int b, int c ){
    return std::max(std::max(a,b),c);
  }

  double min( double a, double b, double c){
    return std::min(std::min(a, b),c);
  }
  double max( double a, double b, double c ){
    return std::max(std::max(a,b),c);
  }
}

namespace faint {

  ColRGB::ColRGB( uchar in_r, uchar in_g, uchar in_b )
    : r(in_r),
      g(in_g),
      b(in_b)
  {}

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

  Color::Color( const ColRGB& rgb, uchar in_a )
    : r(rgb.r),
      g(rgb.g),
      b(rgb.b),
      a(in_a)
  {}

  ColRGB Color::GetRGB() const{
    return ColRGB(r,g,b);
  }

  bool Color::operator==( const Color& c2 ) const{
    return r == c2.r && g == c2.g && b == c2.b && a == c2.a;
  }
  bool Color::operator!=( const Color& c2 ) const{
    return r != c2.r || g != c2.g || b != c2.b || a != c2.a;
  }

  bool Color::operator<( const Color& c2 ) const{
    return to_uint(*this) < to_uint(c2);
  }

  HS::HS( double in_h, double in_s ) :
    h(in_h),
    s(in_s)
  {}

  HSL::HSL( double in_h, double in_s, double in_l ) :
    h(in_h),
    s(in_s),
    l(in_l)
  {
    assert( 0 <= h && h <= 360.0 ); // Fixme: maybe 359.0
    //assert( 0 <= s && s <= 1.0 ); // Fixme: allow some off
    //assert( 0 <= l && l <= 1.0 ); // Fixme: allow some off

  }

  HSL::HSL( const HS& hueSat, double in_l )
    : h(hueSat.h),
      s(hueSat.s),
      l(in_l)
  {}

  HS HSL::GetHS() const{
    return HS(h,s);
  }

  ColRGB blend_alpha( const faint::Color& color, const ColRGB& bg ){
    return rgb_from_ints( ( color.r * color.a + bg.r * ( 255 - color.a ) ) / 255,
      ( color.g * color.a + bg.g * ( 255 - color.a ) ) / 255,
      ( color.b * color.a + bg.b * ( 255 - color.a ) ) / 255 );
  }

  ColRGB strip_alpha( const Color& color ){
    return ColRGB( color.r, color.g, color.b );
  }

  Color color_black(){
    return Color(0,0,0);
  }

  Color color_gray(){
    return Color(128,128,128);
  }

  Color color_magenta(){
    return Color(255,0,255);
  }

  Color color_transparent_black(){
    return Color(0,0,0,0);
  }

  Color color_transparent_white(){
    return Color(0,0,0,0);
  }

  Color color_white(){
    return faint::Color(255,255,255);
  }

  ColRGB rgb_black(){
    return ColRGB(0,0,0);
  }

  int constrained( int lower, int value, int upper ){
    return std::min(std::max(lower, value), upper);
  }

  uint constrained_u( uint lower, uint value, uint upper ){
    return std::min(std::max(lower, value), upper);
  }

  double constrained( double lower, double value, double upper ){
    return std::min( std::max(lower, value), upper );
  }

  Color color_from_ints( int ir, int ig, int ib, int ia ){
    faint::uchar r = static_cast<faint::uchar>(constrained(0,ir,255));
    faint::uchar g = static_cast<faint::uchar>(constrained(0,ig,255));
    faint::uchar b = static_cast<faint::uchar>(constrained(0,ib,255));
    faint::uchar a = static_cast<faint::uchar>(constrained(0,ia,255));
    return Color(r,g,b,a);
  }

  ColRGB grayscale_rgb( int lightness ){
    return rgb_from_ints(lightness, lightness, lightness);
  }

  Color grayscale_rgba( int lightness, int alpha ){
    return color_from_ints(lightness, lightness, lightness, alpha);
  }

  ColRGB rgb_from_ints( int ir, int ig, int ib ){
    faint::uchar r = static_cast<faint::uchar>(constrained(0,ir,255));
    faint::uchar g = static_cast<faint::uchar>(constrained(0,ig,255));
    faint::uchar b = static_cast<faint::uchar>(constrained(0,ib,255));
    return ColRGB(r,g,b);
  }

  Color color_from_uints( uint ir, uint ig, uint ib, uint ia ){
    faint::uchar r = static_cast<faint::uchar>(constrained_u(0,ir,255));
    faint::uchar g = static_cast<faint::uchar>(constrained_u(0,ig,255));
    faint::uchar b = static_cast<faint::uchar>(constrained_u(0,ib,255));
    faint::uchar a = static_cast<faint::uchar>(constrained_u(0,ia,255));
    return Color(r,g,b,a);
  }

  Color color_from_double( double r, double g, double b, double a ){
    return Color(static_cast<faint::uchar>(constrained(0.0, r, 255.0)),
      static_cast<faint::uchar>(constrained(0.0, g, 255.0)),
      static_cast<faint::uchar>(constrained(0.0, b, 255.0)),
      static_cast<faint::uchar>(constrained(0.0, a, 255.0)));
  }

  ColRGB rgb_from_double( double r, double g, double b ){
    return ColRGB(static_cast<faint::uchar>(constrained(0.0, r, 255.0)),
      static_cast<faint::uchar>(constrained(0.0, g, 255.0)),
      static_cast<faint::uchar>(constrained(0.0, b, 255.0)));
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

double dmod( double value, double range ){
  while ( value >= range ){
    value -= range;
  }
  return value;
}

HSL to_hsl( const faint::ColRGB& c ){
  double R = c.r / 255.0;
  double G = c.g / 255.0;
  double B = c.b / 255.0;
  double M = my::max(R, G, B );
  double m = my::min(R, G, B);
  double C = M - m;
  double Hp = 0;

  if ( C == 0 ){
    Hp = 0.0;
  }
  else if ( M == R ){
    Hp = (( ( ( G - B ) / C ) ) ) + 6.0;
  }
  else if ( M == G ){
    Hp = ( (B - R) / C ) + ( 2.0 );
  }
  else if ( M == B ){
    Hp = ( (R - G) / C ) + ( 4.0 );
  }

  double H = dmod( 60.0 * Hp, 360.0 );
  double L = ( M + m ) / 2.0; // Fixme: Consider luma
  double S = C == 0 ? 0 :
    ( C / ( 1.0 - std::fabs(2 * L - 1.0 ) ) );
  return HSL(H, S, L);
}

ColRGB to_rgb( const HSL& c ){
  double C = ( 1.0 - std::fabs( 2 * c.l - 1.0) ) * c.s;
  double Hp = c.h / 60.0;
  double X = C * ( 1 - std::fabs( dmod(Hp, 2.0) - 1.0 ) );

  double R, G, B;
  if ( Hp < 1 ){
    R = C;
    G = X;
    B = 0;
  }
  else if ( Hp < 2 ){
    R = X;
    G = C;
    B = 0;
  }
  else if ( Hp < 3 ){
    R = 0;
    G = C;
    B = X;
  }
  else if ( Hp < 4 ){
    R = 0;
    G = X;
    B = C;
  }
  else if ( Hp < 5 ){
    R = X;
    G = 0;
    B = C;
  }
  else if ( Hp < 6 ){
    R = C;
    G = 0;
    B = X;
  }
  else{
    assert(false);
    return faint::ColRGB(0,0,0);
  }
  double m = c.l - C / 2.0;
  return rgb_from_double(255 * (R + m) + 0.5, 255 * (G + m) + 0.5, 255 * (B + m) + 0.5);
}

Color to_rgba( const HSL& hsl, int a ){
  return Color(to_rgb(hsl),
    static_cast<faint::uchar>(constrained(0,a,255)));
}

} // namespace faint
