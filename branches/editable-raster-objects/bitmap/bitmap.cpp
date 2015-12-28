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

#include <algorithm>
#include <cassert>
#include <cstring>
#include <map>
#include <vector>
#include "bitmap/bitmap.hh"
#include "geo/geotypes.hh"
#include "rendering/cairocontext.hh"
#include "util/gradient.hh"
#include "util/pattern.hh"

using std::swap;
using std::abs;

#define EVER ;;

namespace faint{

const uint bpp = 4;
const int iR = 2;
const int iG = 1;
const int iB = 0;
const int iA = 3;

Bitmap::Bitmap()
  : m_format( INVALID_BMP_FORMAT ),
    m_row_stride(0)
{
  m_w = m_h = 0;
  m_data = nullptr;
}

Bitmap::Bitmap( const Bitmap& other )
  : m_format( other.m_format ),
    m_row_stride( other.m_row_stride ),
    m_w( other.m_w ),
    m_h( other.m_h ),
    m_data(nullptr)
{
  if ( other.m_data != nullptr ){
    m_data = new uchar[ m_row_stride * m_h ];
    memcpy( m_data, other.m_data, m_row_stride * m_h );
  }
}

Bitmap::Bitmap( const IntSize& sz )
  : m_format(ARGB32),
    m_row_stride(0),
    m_w(sz.w),
    m_h(sz.h),
    m_data(nullptr)
{
  assert(m_w > 0);
  assert(m_h > 0);
  int stride = faint_cairo_stride( sz );
  if ( stride == -1 ){
    throw std::bad_alloc(); // Fixme
  }
  m_row_stride = static_cast<uint>(stride);
  m_data = new uchar[ m_row_stride * m_h ];
  memset( m_data, 0, stride * m_h );
}

Bitmap::Bitmap( const IntSize& sz, const Color& bgColor )
  : m_format(ARGB32),
    m_row_stride(0),
    m_w(sz.w),
    m_h(sz.h),
    m_data(nullptr)
{
  assert(m_w > 0);
  assert(m_h > 0);
  int stride = faint_cairo_stride( sz );
  if ( stride == -1 ){
    throw std::bad_alloc(); // Fixme
  }
  m_row_stride = static_cast<uint>(stride);
  m_data = new uchar[ m_row_stride * m_h ];
  clear(*this, bgColor);
}

Bitmap::Bitmap( BitmapFormat format, const IntSize& sz, uint stride )
  : m_format( format ),
    m_row_stride( stride ),
    m_w( static_cast<uint>(sz.w) ),
    m_h( static_cast<uint>(sz.h) ),
    m_data(nullptr)
{
  assert( sz.w > 0 );
  assert( sz.h > 0 );
  m_data = new uchar[ stride * m_h ];
  memset( m_data, 0, stride * m_h );
}

Bitmap::Bitmap( Bitmap&& source ) :
  m_format( source.m_format ),
  m_row_stride( source.m_row_stride ),
  m_w( source.m_w ),
  m_h( source.m_h ),
  m_data( source.m_data )
{
  source.m_data = nullptr;
  source.m_w = 0;
  source.m_h = 0;
}

Bitmap::~Bitmap(){
  delete[] m_data;
}

Bitmap& Bitmap::operator=( const Bitmap& other ){
  if ( &other == this ){
    return *this;
  }
  delete[] m_data;
  m_data = nullptr;
  m_format = other.m_format;
  m_w = other.m_w;
  m_h = other.m_h;
  m_row_stride = other.m_row_stride;
  if ( other.m_data != nullptr ){
    m_data = new uchar[ m_row_stride * m_h ];
    memcpy( m_data, other.m_data, m_row_stride * m_h );
  }
  return *this;
}

Bitmap& Bitmap::operator=( Bitmap&& other ){
  assert(&other != this);
  delete[] m_data;
  m_data = other.m_data;
  other.m_data = nullptr;
  m_format = other.m_format;
  m_row_stride = other.m_row_stride;
  m_w = other.m_w;
  m_h = other.m_h;
  other.m_w = 0;
  other.m_h = 0;
  return *this;
}

Brush::Brush( int w, int h, bool fill )
  : m_w(w),
    m_h(h)
{
  assert(m_w > 0);
  assert(m_h > 0);
  m_data = new uchar[ m_w * m_h ];
  if ( fill ){
    memset( m_data, 255, m_w * m_h );
  }
  else {
    memset( m_data, 0, m_w * m_h );
  }
}

Brush::Brush( const Brush& other ){
  m_w = other.m_w;
  m_h = other.m_h;
  m_data = new uchar[ m_w * m_h ];
  memcpy( m_data, other.m_data, m_w * m_h );
}

Brush::~Brush(){
  delete[] m_data;
}

void Brush::Set( int x, int y, uchar value ){
  m_data[ y * m_w + x ] = value;
}

uchar Brush::Get( int x, int y ) const{
  return m_data[ y * m_w + x ];
}

} // namespace faint

namespace bmp_impl{
using namespace faint;
class color_ptr{
public:
  // For fetching rgba values from an uchar-pointer
  // (Which must be at the correct four-byte offset)
  uchar& r;
  uchar& g;
  uchar& b;
  uchar& a;
  color_ptr( uchar* p ):
    r(*(p + iR)),
    g(*(p + iG)),
    b(*(p + iB)),
    a(*(p + iA))
  {}
  void Set( const Color& c ){
    r = c.r;
    g = c.g;
    b = c.b;
    a = c.a;
  }
private:
  color_ptr& operator=(const color_ptr&); // Prevent assignment
};

class const_color_ptr{
public:
  // For fetching rgba values from an uchar-pointer
  // (Which must be at the correct four-byte offset)
  const uchar r;
  const uchar g;
  const uchar b;
  const uchar a;
  const_color_ptr( const uchar* p ):
    r(*(p + iR)),
    g(*(p + iG)),
    b(*(p + iB)),
    a(*(p + iA))
  {}
private:
  const_color_ptr& operator=(const const_color_ptr&); // Prevent assignment
};

bool operator==(const color_ptr& c1, const Color& c2 ){
  return c1.r == c2.r &&
    c1.g == c2.g &&
    c1.b == c2.b &&
    c1.a == c2.a;
}

inline int wrap( int v, int n ){
  return v >= 0 ?
    v % n :
    ( abs(v) * (n - 1) ) % n;
}

Color get_color_modulo_raw( const Bitmap& bmp, int x, int y ){
  return get_color_raw( bmp, wrap(x, bmp.m_w), wrap(y, bmp.m_h ) );
}

Color get_color_modulo( const Bitmap& bmp, const IntPoint& pos ){
  return get_color_raw( bmp, wrap(pos.x, bmp.m_w), wrap(pos.y, bmp.m_h) );
}

static void blend_pixel_raw( Bitmap& dst, int x, int y, const Color& c ){
  // Fixme: Probably dumb function hacked together while testing blending of radial gradients
  if ( x < 0 || y < 0 || x >= dst.m_w || y >= dst.m_h ) {
    return;
  }
  uchar* dstData = dst.GetRaw();
  int pos = y * dst.m_row_stride + x * 4;

  dstData[pos + iR] = static_cast<uchar>((c.r * c.a + dstData[pos + iR] * ( 255 - c.a )) / 255);
  dstData[pos + iG] = static_cast<uchar>((c.g * c.a + dstData[pos + iG] * ( 255 - c.a )) / 255);
  dstData[pos + iB] = static_cast<uchar>((c.b * c.a + dstData[pos + iB] * ( 255 - c.a )) / 255);
}

static int int_abs( int v ){
  return v < 0 ? -v : v;
}

static void sort_y( coord& x0, coord& y0, coord& x1, coord& y1, coord& x2, coord& y2 ){
  if ( y1 > y2 ){
    swap( x1, x2 );
    swap( y1, y2 );
  }
  if ( y0 > y1 ){
    swap( x0, x1 );
    swap( y0, y1 );
  }
  if ( y1 > y2 ){
    swap( x1, x2 );
    swap( y1, y2 );
  }

  assert( y0 <= y1 && y1 <= y2 );
}

static IntPoint get_offset( int x0, int y0, int x1, int y1, int lineWidth ){
  const bool steep = int_abs(y1 - y0) > int_abs( x1 - x0 );
  if ( steep ){
    swap(x0,y0);
    swap(x1,y1);
  }
  if (x0 > x1){ // Not implemented
    swap(x0, x1);
    swap(y0,y1);
  }
  int dx = x1 - x0;
  int dy = int_abs(y1 - y0);
  int p = 2 * dy - int_abs(dx);
  int yStep = y0 < y1 ? -1 : 1;
  int at_x = 0;
  int at_y = 0;
  int yp = 0;

  coord distance = 0;
  coord diag = sqrt(2.0);
  for ( int xp = 0; xp != lineWidth; xp++ ){
    at_x = - yp;
    at_y = - xp;
    if ( p < 0 ){
      p += 2*dy;
      distance += 1;
    }
    else {
      yp += yStep;
      p += 2*dy - 2*dx;
      distance += diag;
    }
    if ( distance >= lineWidth ){
      break;
    }
  }
  if ( steep ){
    return IntPoint( at_y, at_x );
  }
  return IntPoint( at_x, at_y );
}

inline bool inside( const DstBmp& dst, const Bitmap& src, int x0, int y0 ){
  IntSize sz(dst.GetSize());
  if ( x0 >= sz.w || y0 >= sz.h ){
    return false;
  }
  if ( x0 + src.m_w < 0 || y0 + src.m_h < 0 ){
    return false;
  }
  return true;
}

std::map<int,int> ellipse_points( int x0, int y0, int a, int b ){
  if ( a == 0 || b == 0 ){
    return std::map<int,int>();
  }

  std::map<int,int> points;
  int a2 = 2 * a * a;
  int b2 = 2 * b * b;
  int error = a * a * b;
  int x = 0;
  int y = b;
  int stopy = 0;
  int stopx = a2 * b;

  while (stopy <= stopx) {
    //put_pixel( bmp, x0 + x, y0 + y, c );
    points[ y0 + y ] = x0 - x;
    points[ y0 - y ] = x0 - x;
    //put_pixel( bmp, x0 + x, y0 - y, c );
    x++;

    error -= b2 * (x - 1);
    stopy += b2;
    if (error <= 0) {
      error += a2 * (y - 1);
      y--;
      stopx -= a2;
    }
  }

  error = b*b*a;
  x = a;
  y = 0;
  stopy = b2*a;
  stopx = 0;
  while (stopy >= stopx) {

    //put_pixel( bmp, x0 + x, y0 + y, c);
    points[ y0 + y ] = x0 - x;
    points[ y0 - y ] = x0 - x;
    // put_pixel( bmp, x0 + x, y0 - y, c);
    y++;
    error -= a2 * (y - 1);
    stopx += a2;
    if (error < 0) {
      error += b2 * (x - 1);
      x--;
      stopy -= b2;
    }
  }
  return points;
}

template<typename Functor>
void scanline_f( Bitmap& bmp, int x0, int x1, int y, const Functor& setPixFunc ){
  if ( x0 > x1 ){
    swap(x0,x1);
  }
  for ( int x = x0; x <= x1; x++ ){
    setPixFunc(bmp, x, y);
  }
}

template<typename Functor>
void scanline_fl_f( Bitmap& bmp, coord x0, coord x1, coord y, const Functor& setPixFunc ){
  scanline_f( bmp, floored(x0), floored(x1), floored(y), setPixFunc );
}

template<typename Functor>
void vertical_f( Bitmap& bmp, int y0, int y1, int x, const Functor& setPixFunc ){
  if ( y0 > y1 ){
    swap(y0,y1);
  }
  for ( int y = y0; y <= y1; y++ ){
    setPixFunc(bmp, x, y);
  }
}

template<typename Functor>
void line_circle_f( Bitmap& bmp, const Functor& setPixFunc, const IntPoint& p, int lineWidth ){
  int halfLw = ( lineWidth - 1 ) / 2;
  const IntPoint e0( -halfLw, -halfLw );
  const IntPoint e1(halfLw, halfLw);
  fill_ellipse_f( bmp, setPixFunc, IntRect(p + e0, p + e1) );
}

template<typename Functor>
void draw_wide_line_f( Bitmap& bmp, const IntPoint& p0, const IntPoint& p1, const Functor& setPixFunc, int lineWidth, bool dashes, LineCap cap ){
  int x0 = p0.x;
  int y0 = p0.y;
  int x1 = p1.x;
  int y1 = p1.y;
  const bool steep = int_abs(y1 - y0) > int_abs( x1 - x0 );
  if ( steep ){
    swap(x0,y0);
    swap(x1,y1);
  }
  if (x0 > x1){
    swap(x0, x1);
    swap(y0,y1);
  }

  const int dx = x1 - x0;
  const int dy = int_abs(y1 - y0);
  const int yStep = y0 < y1 ? -1 : 1;
  int p = 2 * dy - int_abs(dx); // Error for the line
  int pp_o = p; // Outer-error for the parallel line (p-line)

  // Use a bounding-line as kind of dumb method to bound the p-lines
  // so that the opposite edge stays non-jagged, and steps in the
  // same manner as the start line
  const IntPoint pt = get_offset(x0,y0, x1, y1, lineWidth );
  int xMin = x0 + pt.x * yStep; // x-boundary for the p-lines
  int yMin = y0 + pt.y * yStep; // y-boundary for the p-lines
  int p2 = p; // Error for the boundary

  // The p-lines are shifted to the left for each "jump" in the p-line
  // due to jumps in the regular line
  int xOff = 0;
  int y = 0; // Jumps in the line

  // Loop along the regular line
  if ( dx == 0 ){
    return;
  }
  bool on = false;
  int steps = 0;
  for ( int i = 0; i - 1 != dx - xOff; i++ ){
    if ( steps == 0 ){
      on = !dashes || !on;
    }
    steps = (steps + 1) % (lineWidth * 2);
    int pp = pp_o; // Offset the error for this p-line for this "height"
    int yp = 0; // Jumps in the p-line
    int xStart = x0 + i + xOff;
    int yStart = y0 - y;
    int at_x;
    int at_y;

    // For each i, draw a parallel line
    int xp = 0;
    for (EVER) {
      at_x = xStart - yp * yStep;
      at_y = yStart - xp * yStep;
      if ( at_x < xMin ){
        // Break without setting a pixel if about to jump across
        // a finished segment
        break;
      }
      assert( xp <= lineWidth );
      if ( steep ){
        if ( on ){
          setPixFunc( bmp, at_y - yStep * pt.y / 2, at_x - yStep * pt.x / 2 );
        }
      }
      else{
        if ( on ){
          setPixFunc( bmp, at_x - yStep * pt.x / 2, at_y - yStep * pt.y / 2 );
        }
      }
      if ( (yStep == 1 && at_y <= yMin) || (yStep == -1 && at_y >= yMin ) ){
        // Break after setting the final pixel
        break;
      }
      if ( pp < 0 ){
        pp += 2*dy;
      }
      else {
        yp += yStep;
        pp += 2*dy - 2*dx;
      }
      xp++;
    }

    // Update the bounding line when it's been hit
    if ( at_x == xMin && at_y == yMin ){
      xMin += 1;
      if ( p2 < 0 ){
        p2 += 2 * dy;
      }
      else {
        yMin -= yStep;
        p2 += 2 * dy - 2*dx;
      }
    }

    // Update the start line
    if ( p < 0 ){
      p += 2*dy;
    }
    else {
      y += yStep;
      p += 2*dy - 2*dx;
      if ( pp_o < 0 ){
        pp_o += 2*dy;
      }
      else {
        pp_o += 2*dy - 2*dx;
        p -= 2 * dy;
        xOff -= 1;
      }
    }
  }

  if ( cap == LineCap::ROUND ){
    line_circle_f(bmp, setPixFunc, p0, lineWidth );
    line_circle_f(bmp, setPixFunc, p1, lineWidth );
  }
}

inline void brush_stroke( Bitmap& bmp, int x, int y, const Brush& b, const Color& c ){
  int xCenter = b.m_w / 2;
  int yCenter = b.m_h / 2;
  for ( int yB = 0; yB != b.m_h; yB++ ){
    for ( int xB = 0; xB != b.m_w; xB++ ){
      if ( b.Get( xB, yB ) > 10 ){
        int xD = x + xB - xCenter;
        int yD = x + xB - yCenter;
        if ( xD < 0 || yD < 0 ){
          continue;
        }
        put_pixel_raw( bmp, x + xB - xCenter, y + yB - yCenter, c );
      }
    }
  }
}

struct ColorFromColor{
  ColorFromColor( const Color& color )
    : m_color(color)
  {}
  void operator()( Bitmap& dst, int x, int y ) const{
    put_pixel_raw(dst, x, y, m_color );
  }
  Color m_color;
};

struct ColorFromGradient{
  // Functor for using a Gradient as draw source
  // Fixme: Use new ColorFromBitmap with both Pattern and Gradient
  ColorFromGradient(const Gradient& g, const IntRect& r )
    : m_bmp(cairo_gradient_bitmap(g, r.GetSize())),
      m_x(-r.x),
      m_y(-r.y)
  {}
  void operator()( Bitmap& dst, int x, int y ) const{
    // Fixme: Blend or set should depend on ts_AlphaBlending
    blend_pixel_raw(dst, x, y, get_color_modulo_raw( m_bmp, x + m_x, y + m_y ) );
  }
  Bitmap m_bmp;
  int m_x;
  int m_y;
private:
  ColorFromGradient& operator=(const ColorFromGradient&); // Silence warning
};

struct ColorFromPattern{
  // Functor for using a bitmap as draw source
  ColorFromPattern( const Pattern& pattern )
    : m_bmp(pattern.GetBitmap())
  {
    const IntPoint offset = pattern.GetAnchor();
    m_x = offset.x;
    m_y = offset.y;
  }
  void operator()( Bitmap& dst, int x, int y ) const{
    // Fixme: Blend or Set should depend on ts_AlphaBlending
    blend_pixel_raw(dst, x, y, get_color_modulo_raw( m_bmp, x + m_x, y + m_y) );
  }
  const Bitmap& m_bmp;
  int m_x;
  int m_y;
private:
  ColorFromPattern& operator=(const ColorFromPattern&); // Silence warning
};

static void clear_pattern( Bitmap& bmp, const Pattern& pattern ){
  const IntSize sz(bmp.GetSize());
  const Bitmap& srcBmp(pattern.GetBitmap());
  const IntSize patSz(srcBmp.GetSize());
  for ( int y = 0; y != sz.h; y++ ){
    for ( int x = 0; x != sz.w; x++ ){
      put_pixel_raw( bmp, x, y, get_color_modulo_raw( srcBmp, x, y) );
    }
  }
}

template<typename Functor>
void draw_wide_ellipse_f( Bitmap& bmp, const IntRect& r, int lineWidth, const Functor& setPixFunc ){
  IntPoint radiuses(floored(floated(IntPoint(r.w, r.h)) / LITCRD(2.0)));
  int x0 = r.x + radiuses.x;
  int y0 = r.y + radiuses.y;
  int a = radiuses.x;
  int b = radiuses.y;

  if ( a == 0 ){
    draw_line_f( bmp, setPixFunc, r.TopLeft(), r.BottomLeft(), lineWidth, false, LineCap::BUTT );
    return;
  }
  if ( b == 0 ){
    draw_line_f( bmp, setPixFunc, r.TopLeft(), r.TopRight(), lineWidth, false, LineCap::BUTT );
    return;
  }

  std::map<int,int> points = ellipse_points( x0, y0, a - lineWidth, b - lineWidth );
  for ( auto ptPair : points ){
    setPixFunc( bmp, ptPair.second, ptPair.first );
  }

  int a2 = 2 * a * a;
  int b2 = 2 * b * b;
  int error = a * a * b;

  int x = 0;
  int y = b;

  int xoffset = r.w % 2 == 0 ? 1 : 0;
  int yoffset = r.h % 2 == 0 ? 1 : 0;

  int stopy = 0;
  int stopx = a2 * b;

  while (stopy <= stopx) {
    setPixFunc( bmp, x0 + x - xoffset, y0 + y - yoffset );

    if ( points.find( y0 + y ) != points.end() ){
      int x1 = points[y0 + y];
      scanline_f( bmp, x0 - x, x1, y0 + y - yoffset, setPixFunc );
      int dx = x1 - ( x0 - x );
      scanline_f( bmp, x0 + x - dx - xoffset, x0 + x - xoffset, y0 + y - yoffset, setPixFunc );
    }
    else {
      scanline_f( bmp, x0 - x, x0 + x - xoffset, y0 + y - yoffset, setPixFunc );
    }

    if ( points.find( y0 - y ) != points.end() ){
      int x1 = points[y0 - y];
      scanline_f( bmp, x0 - x, x1, y0 - y, setPixFunc );
      int dx = x1 - ( x0 - x );
      scanline_f( bmp, x0 + x - xoffset - dx, x0 + x - xoffset, y0 - y, setPixFunc);
    }
    else {
      scanline_f( bmp, x0 - x, x0 + x - xoffset, y0 - y, setPixFunc );
    }
    x++;

    error -= b2 * (x - 1);
    stopy += b2;
    if (error <= 0) {
      error += a2 * (y - 1);
      y--;
      stopx -= a2;
    }
  }

  error = b*b*a;
  x = a;
  y = 0;
  stopy = b2*a;
  stopx = 0;
  while (stopy >= stopx) {
    if ( points.find( y0 + y ) != points.end() ){
      int x1 = points[y0 + y];
      scanline_f( bmp, x0 - x, x1, y0 + y - yoffset, setPixFunc );
      int dx = x1 - ( x0 - x );
      scanline_f( bmp, x0 + x - xoffset - dx, x0 + x - xoffset, y0 + y - yoffset, setPixFunc );
    }
    else {
      scanline_f( bmp, x0 - x, x0 + x - xoffset, y0 + y - yoffset, setPixFunc );
    }

    if ( points.find( y0 - y ) != points.end() ){
      int x1 = points[y0 - y];
      scanline_f( bmp, x0 - x, x1, y0 - y, setPixFunc );
      int dx = x1 - ( x0 - x );
      scanline_f( bmp, x0 + x - xoffset - dx, x0 + x - xoffset, y0 - y, setPixFunc );
    }
    else {
      scanline_f( bmp, x0 - x, x0 + x - xoffset, y0 - y, setPixFunc );
    }
    y++;
    error -= a2 * (y - 1);
    stopx += a2;
    if (error < 0) {
      error += b2 * (x - 1);
      x--;
      stopy -= b2;
    }
  }
}

template<typename Functor>
void draw_ellipse_f( Bitmap& bmp, const Functor& setPixFunc, const IntRect& r, int lineWidth, bool dashes ){
  if ( lineWidth > 1 ){
    draw_wide_ellipse_f( bmp, r, lineWidth, setPixFunc );
    return;
  }

  IntPoint radiuses(floored(floated(IntPoint(r.w, r.h)) / LITCRD(2.0)));
  int x0 = r.x + radiuses.x;
  int y0 = r.y + radiuses.y;
  int a = radiuses.x;
  int b = radiuses.y;

  if ( a == 0 ){
    draw_line_f( bmp, setPixFunc, r.TopLeft(), r.BottomLeft(), 1, dashes, LineCap::BUTT );
    return;
  }
  if ( b == 0 ){
    draw_line_f( bmp, setPixFunc, r.TopLeft(), r.TopRight(), 1, dashes, LineCap::BUTT );
    return;
  }

  int a2 = 2 * a * a;
  int b2 = 2 * b * b;
  int error = a * a * b;
  int x = 0;
  int y = b;

  int stopy = 0;
  int stopx = a2 * b;

  int xoffset = r.w % 2 == 0 ? 1 : 0;
  int yoffset = r.h % 2 == 0 ? 1 : 0;
  bool on = false;
  int steps = 0;
  while (stopy <= stopx) {
    if ( steps == 0 ){
      on = !dashes || !on;
    }
    steps = (steps + 1) % 2;
    if ( on ){

      setPixFunc( bmp, x0 + x - xoffset, y0 + y - yoffset );
      setPixFunc( bmp, x0 - x, y0 + y - yoffset );
      setPixFunc( bmp, x0 - x, y0 - y );
      setPixFunc( bmp, x0 + x - xoffset, y0 - y );
    }
    x++;

    error -= b2 * (x - 1);
    stopy += b2;
    if (error <= 0) {
      error += a2 * (y - 1);
      y--;
      stopx -= a2;
    }
  }

  error = b*b*a;
  x = a;
  y = 0;
  stopy = b2*a;
  stopx = 0;
  while (stopy >= stopx) {
    if ( steps == 0 ){
      on = !dashes || !on;
    }
    steps = (steps + 1) % 2;
    if ( on ){
      setPixFunc( bmp, x0 + x - xoffset, y0 + y - yoffset );
      setPixFunc( bmp, x0 - x, y0 + y - yoffset );
      setPixFunc( bmp, x0 - x, y0 - y );
      setPixFunc( bmp, x0 + x - xoffset, y0 - y );
    }
    y++;
    error -= a2 * (y - 1);
    stopx += a2;
    if (error < 0) {
      error += b2 * (x - 1);
      x--;
      stopy -= b2;
    }
  }
}

template<typename Functor>
void draw_line_f( Bitmap& bmp, const Functor& setPixFunc, const IntPoint& p0, const IntPoint& p1, int lineWidth, bool dashes, LineCap cap ){
  lineWidth = std::max(1, lineWidth);
  if ( lineWidth > 1 ){
    draw_wide_line_f( bmp, p0, p1, setPixFunc, lineWidth, dashes, cap );
    return;
  }

  int x0 = p0.x;
  int y0 = p0.y;
  int x1 = p1.x;
  int y1 = p1.y;
  const bool steep = int_abs( y1 - y0 ) > int_abs( x1 - x0 );
  if ( steep ){
    swap( x0, y0 );
    swap( x1, y1 );
  }
  if ( x0 > x1 ){
    swap( x0, x1 );
    swap( y0, y1 );
  }

  int dx = x1 - x0;
  int dy = int_abs(y1 - y0);
  coord err = 0;
  int y = y0;
  int yStep = y0 < y1 ? 1 : -1;

  bool on = false;
  int steps = 0;
  for ( int x = x0; x<= x1; x++ ){
    if ( steps == 0 ){
      on = !dashes || !on;
    }
    steps = ( steps + 1 ) % 2;

    err += dy;
    if ( steep ){
      if ( on ){
        setPixFunc( bmp, y, x );
      }
    }
    else {
      if ( on ){
        setPixFunc( bmp, x, y );
      }
    }
    if ( 2 * err > dx ){
      y += yStep;
      err -= dx;
    }
  }
}

template<typename Functor>
void draw_polygon_f( Bitmap& bmp, const Functor& setPixFunc, const std::vector<IntPoint>& points, int lineWidth, bool dashes ){
  if ( points.empty() ){
    return;
  }
  if ( points.size() == 1 ){
    const IntPoint& p(points[0]);
    draw_line_f( bmp, setPixFunc, p, p, lineWidth, dashes, LineCap::ROUND );
  }

  for ( size_t i = 1; i != points.size(); i++ ){
    draw_line_f( bmp, setPixFunc, points[ i - 1], points[i], lineWidth, dashes, LineCap::BUTT );
    line_circle_f( bmp, setPixFunc, points[i], lineWidth );
  }
  draw_line_f( bmp, setPixFunc, points.back(), points[0], lineWidth, dashes, LineCap::BUTT );
  line_circle_f( bmp, setPixFunc, points[0], lineWidth );
}

template<typename Functor>
void draw_polyline_f( Bitmap& bmp, const Functor& setPixFunc, const std::vector<IntPoint>& points, int lineWidth, bool dashes, LineCap cap ){
  if ( points.empty() ){
    return;
  }
  if ( points.size() == 1 ){
    const IntPoint& p(points[0]);
    draw_line_f( bmp, setPixFunc, p, p, lineWidth, dashes, cap );
  }

  for ( size_t i = 1; i != points.size(); i++ ){
    draw_line_f( bmp, setPixFunc, points[ i - 1], points[i], lineWidth, dashes, LineCap::BUTT );
    if ( i != points.size() - 1 ){
      line_circle_f( bmp, setPixFunc, points[i], lineWidth );
    }
  }
  if ( cap == LineCap::ROUND ){
    line_circle_f( bmp, setPixFunc, points[0], lineWidth );
    line_circle_f( bmp, setPixFunc, points.back(), lineWidth );
  }
}

template<typename Functor>
void draw_rect_f( Bitmap& bmp, const Functor& setPixFunc, const IntRect& r, int lineWidth, bool dashes ){
  if ( !dashes ){
    lineWidth = std::max(1, lineWidth);
    for ( int i = 0; i != lineWidth; i++ ){
      scanline_f( bmp, r.Left(), r.Right(), r.Top() + i, setPixFunc );
      scanline_f( bmp, r.Left(), r.Right(), r.Bottom() - i, setPixFunc );
      vertical_f( bmp, r.Top(), r.Bottom(), r.Left() + i, setPixFunc );
      vertical_f( bmp, r.Top(), r.Bottom(), r.Right() - i, setPixFunc );
    }
  }
  else {
    draw_polyline_f( bmp, setPixFunc, as_line_path(r), lineWidth, true, LineCap::BUTT );
  }
}

template<typename Functor>
void fill_ellipse_f( Bitmap& bmp, const Functor& setPixFunc, const IntRect& r ){
  IntPoint radiuses(floored(floated(IntPoint(r.w, r.h)) / LITCRD(2.0)));
  int x0 = r.x + radiuses.x;
  int y0 = r.y + radiuses.y;
  int a = radiuses.x;
  int b = radiuses.y;

  if ( a == 0 ){
    vertical_f( bmp, r.Top(), r.Bottom(), r.Left(), setPixFunc);
    return;
  }
  if ( b == 0 ){
    scanline_f( bmp, r.Left(), r.Right(), r.Top(), setPixFunc );
    return;
  }
  int a2 = 2 * a * a;
  int b2 = 2 * b * b;
  int error = a * a * b;

  int x = 0;
  int y = b;

  int stopy = 0;
  int stopx = a2 * b;

  int xoffset = r.w % 2 == 0 ? 1 : 0;
  int yoffset = r.h % 2 == 0 ? 1 : 0;

  while (stopy <= stopx) {
    scanline_f( bmp, x0 - x, x0 + x - xoffset, y0 + y - yoffset, setPixFunc );
    if ( y != 0 ){
      scanline_f( bmp, x0 - x, x0 + x - xoffset, y0 - y, setPixFunc );
    }
    x++;

    error -= b2 * (x - 1);
    stopy += b2;
    if (error <= 0) {
      error += a2 * (y - 1);
      y--;
      stopx -= a2;
    }
  }

  error = b*b*a;
  x = a;
  y = 0;
  stopy = b2*a;
  stopx = 0;
  while (stopy >= stopx) {
    scanline_f( bmp, x0 - x, x0 + x - xoffset, y0 + y - yoffset, setPixFunc );
    if ( y != 0 ){
      scanline_f( bmp, x0 - x, x0 + x - xoffset, y0 - y, setPixFunc );
    }
    y++;
    error -= a2 * (y - 1);
    stopx += a2;
    if (error < 0) {
      error += b2 * (x - 1);
      x--;
      stopy -= b2;
    }
  }
}

template<typename Functor>
void fill_polygon_f( Bitmap& bmp, const Functor& setPixFunc, const std::vector<IntPoint>& inPoints  ){
  if ( inPoints.empty() ){
    return;
  }
  IntRect r = bounding_rect( inPoints );
  std::vector<IntPoint> points(inPoints);
  points.push_back(points[0]);
  int minX = r.Left() - 1; // Fixme
  int maxX = std::min(r.Right(), bmp.m_w - 1);
  int minY = std::max(0, r.Top());
  int maxY = std::min( r.Bottom(), bmp.m_h - 1);
  for ( int y = minY; y <= maxY; y++ ){
    std::vector<int> x_vals;
    for ( size_t i = 0; i < points.size() - 1; i+= 1 ){
      IntPoint p0 = points[i];
      IntPoint p1 = points[i + 1];
      int x0 = p0.x;
      int x1 = p1.x;
      int y0 = p0.y;
      int y1 = p1.y;
      if ( x0 > x1 ){
        std::swap(x0,x1);
        std::swap(y0,y1);
      }
      if ( ( y0 < y && y <= y1 ) || ( y1 < y && y <= y0 ) ){
        if ( x0 == x1 ){
          x_vals.push_back(x0);
          continue;
        }
        coord k = coord( y1 - y0 ) / ( x1 - x0 );
        coord m = y0 - k * x0;
        int x = static_cast<int>((y - m) / k);
        x_vals.push_back(x);
      }
    }
    if ( x_vals.empty() ){
      continue;
    }
    std::sort(x_vals.begin(), x_vals.end());
    for ( int x = minX; x <= maxX; x++ ){
      for ( size_t j = 0; j != x_vals.size(); j++ ){
        if ( x < x_vals[j] ){
          if ( ( x_vals.size() - j ) % 2 != 0 ) {
            setPixFunc( bmp, x + 1, y ); // Fixme: Why + 1? (Added to fill the insides of polygon edge in gradient panel)
          }
          break;
        }
      }
    }
  }
}

template<typename Functor>
void fill_rect_f( Bitmap& bmp, const Functor& setPixFunc, const IntRect& r ){
  const int x0 = r.x;
  const int y0 = r.y;
  const int x1 = r.Right();
  const int y1 = r.Bottom();

  for ( int y = y0; y <= y1; y++ ){
    for ( int x = x0; x <= x1; x++ ){
      setPixFunc( bmp, x, y );
    }
  }
}

template<typename Functor>
void fill_triangle_f( Bitmap& bmp, const Functor& setPixFunc, const Point& p0, const Point& p1, const Point& p2 ){
  coord x0 = p0.x;
  coord y0 = p0.y;
  coord x1 = p1.x;
  coord y1 = p1.y;
  coord x2 = p2.x;
  coord y2 = p2.y;

  // Fixme: Why floating point type?
  sort_y( x0, y0, x1, y1, x2, y2 );
  assert( y0 <= y1 && y1 <= y2 );
  coord dx1 = ( y1 - y0 > 0 ) ? ( x1 - x0 ) / ( y1 - y0 ) : 0;
  coord dx2 = ( y2 - y0 > 0 ) ? ( x2 - x0 ) / ( y2 - y0 ) : 0;
  coord dx3 = ( y2 - y1 > 0 ) ? ( x2 - x1 ) / ( y2 - y1 ) : 0;

  coord right = x0;
  coord y = y0;
  coord x = x0;

  if ( dx1 > dx2 ){
    for ( ; y < y1; y++, x += dx2, right += dx1 ){
      scanline_fl_f( bmp, x + 0.5, right + 0.5, y + 0.5, setPixFunc );
    }
    right = x1;
    for(; y < y2; y++, x +=dx2,right += dx3){
      scanline_fl_f( bmp, x + 0.5, right + 0.5, y + 0.5, setPixFunc );
    }
  }
  else {
    for ( ; y < y1; y++, x+= dx1, right += dx2 ){
      scanline_fl_f( bmp, x + 0.5, right + 0.5, y + 0.5, setPixFunc );
    }
    x = x1;
    for ( ; y < y2; y++, x += dx3, right += dx2 ){
      scanline_fl_f( bmp, x + 0.5, right + 0.5, y + 0.5, setPixFunc );
    }
  }
}

Bitmap flip_horizontal( const Bitmap& src ){
  Bitmap dst( src );
  uchar* pDst = dst.m_data;
  uchar* pSrc = src.m_data;
  for ( int y = 0; y != src.m_h; y++ ){
    for ( int x = 0 ; x != src.m_w; x++ ){
      int iSrc = y * src.m_row_stride + x * 4;
      int iDst = y * dst.m_row_stride + ( dst.m_w - 1 ) * 4 - x * 4;
      pDst[ iDst + iR ] = pSrc[iSrc + iR ];
      pDst[ iDst + iG ] = pSrc[iSrc + iG ];
      pDst[ iDst + iB ] = pSrc[iSrc + iB ];
      pDst[ iDst + iA ] = pSrc[iSrc + iA ];
    }
  }
  return dst;
}

Bitmap flip_vertical( const Bitmap& src ){
  Bitmap dst( src );
  uchar* pDst = dst.m_data;
  uchar* pSrc = src.m_data;
  for ( int y = 0; y != src.m_h; y++ ){
    for ( int x = 0 ; x != src.m_w; x++ ){
      int iSrc = y * src.m_row_stride + x * 4;
      int iDst = ( dst.m_h - y - 1 )* dst.m_row_stride + x * 4;
      pDst[ iDst + iR ] = pSrc[iSrc + iR ];
      pDst[ iDst + iG ] = pSrc[iSrc + iG ];
      pDst[ iDst + iB ] = pSrc[iSrc + iB ];
      pDst[ iDst + iA ] = pSrc[iSrc + iA ];
    }
  }
  return dst;
}

template<typename Functor, typename Condition>
void set_pixels_if( Bitmap& bmp, const Functor& setPixFunc, const Condition& condition ){
  IntSize sz(bmp.GetSize());
  for ( int y = 0; y != sz.h; y++ ){
    for ( int x = 0; x != sz.w; x++ ){
      if ( condition(x,y) ){
        setPixFunc(bmp, x, y);
      }
    }
  }
}

struct IsColor{
  IsColor( const Bitmap& bmp, const Color& c )
    : m_bmp(bmp),
      m_color(c)
  {}
  bool operator()( int x, int y ) const{
    return get_color_raw( m_bmp, x, y ) == m_color;
  }
  const Bitmap& m_bmp;
  const Color m_color;
private:
  IsColor& operator=( const IsColor& ); // Silence warning
};

} // namespace bmp_impl

// This macro is used to dispatch calls to the correct
// Bitmap-modifying template function depending on the content of a
// DrawSource.  The macro must be used in a scope with a DrawSource
// named "src" and a Bitmap named bmp.
//
// The macro arguments are:
//  1) The unqualified name of the template function
//  2) expression yielding a ColorFromColor
//  3) expression yielding a ColorFromPattern
//  4) expression yielding a ColorFromGradient
//  5+) additional arguments for the dispatched to function.
//
// The dispatched to function must be a template function, on this format:
// template<typename Functor>
// void function(Bitmap&, const Functor&, .. additional arguments.. ).
//
// Implementation note: the explicit template specification used
// within the macro (e.g. func<ColorFromColor>) makes confused order
// of functors in the macro call a compile time error - otherwise this
// would cause a runtime crash for an attempt to instantiate for
// example a ColorFromPattern with a DrawSource containing a gradient.
#define DISPATCH(func, COLORFUNC, PATTERNFUNC, GRADIENTFUNC, ... )\
if  (src.IsColor()){ \
  func<ColorFromColor>(bmp,COLORFUNC, __VA_ARGS__); \
} \
else if ( src.IsPattern() ){\
  func<ColorFromPattern>(bmp, PATTERNFUNC, __VA_ARGS__); \
}\
else if ( src.IsGradient() ){\
  func<ColorFromGradient>(bmp, GRADIENTFUNC, __VA_ARGS__); \
}\
else{ \
  assert(false); \
}
// End of DISPATCH macro

namespace faint{
using namespace bmp_impl;

Bitmap alpha_blended( const Bitmap& src, const Color& color ){
  Bitmap dst( src.GetSize(), color );
  blend( src, onto(dst), IntPoint(0,0) );
  return dst;
}

Bitmap alpha_blended( const Bitmap& src, const faint::ColRGB& c ){
  return alpha_blended( src, faint::Color(c, 255) );
}

bool bitmap_ok( const Bitmap& bmp ){
  return bmp.m_w != 0 && bmp.m_h != 0 && bmp.m_format != INVALID_BMP_FORMAT;
}

void blend_alpha( Bitmap& bmp, const Color& color ){
  bmp = alpha_blended( bmp, color );
}

void blend( const Bitmap& src, DstBmp dst, const IntPoint& topLeft ){
  int x0 = topLeft.x;
  int y0 = topLeft.y;
  if ( !bmp_impl::inside( dst, src, x0, y0 ) ){
    return;
  }

  const uint xMin = std::max( 0, -x0 );
  const uint yMin = std::max( 0, -y0 );
  IntSize dstSz(dst.GetSize());
  const uint xMax = std::min( (int)dstSz.w - x0, (int)src.m_w );
  const uint yMax = std::min( (int)dstSz.h - y0, (int)src.m_h );

  const uint srcStride = src.GetStride();
  const uint dstStride = dst.GetStride();
  const uchar* srcData = src.GetRaw();
  uchar* dstData = dst.GetRaw();
  for ( uint y = yMin; y != yMax; y++ ){
    for ( uint x = xMin; x != xMax; x++ ){
      int srcPos = y * srcStride + x * 4;
      int dstPos = (y + y0) * dstStride + ( x + x0 ) * 4;
      float alpha = srcData[srcPos + iA];
      dstData[dstPos + iR] = static_cast<uchar>((srcData[srcPos + iR] * alpha + dstData[dstPos + iR] * ( 255 - alpha )) / 255);
      dstData[dstPos + iG] = static_cast<uchar>((srcData[srcPos + iG] * alpha + dstData[dstPos + iG] * ( 255 - alpha ) ) / 255);
      dstData[dstPos + iB] = static_cast<uchar>((srcData[srcPos + iB] * alpha + dstData[dstPos + iB] * ( 255 - alpha )) / 255);
      dstData[dstPos + iA] = 255;
    }
  }
}

void blend_masked( const Bitmap& src, DstBmp dst, const Color& maskColor, const IntPoint& topLeft ){
  int x0 = topLeft.x;
  int y0 = topLeft.y;
  if ( !bmp_impl::inside( dst, src, x0, y0 ) ){
    return;
  }

  IntSize dstSz(dst.GetSize());
  const uint xMin = std::max( 0, -x0 );
  const uint yMin = std::max( 0, -y0 );
  const uint xMax = std::min( (int)dstSz.w - x0, (int)src.m_w );
  const uint yMax = std::min( (int)dstSz.h - y0, (int)src.m_h  );

  uint srcStride = src.GetStride();
  uint dstStride = dst.GetStride();
  const uchar* srcData = src.GetRaw();
  uchar* dstData = dst.GetRaw();

  for ( uint y = yMin; y != yMax; y++ ){
    for ( uint x = xMin; x != xMax; x++ ){
      int srcPos = y * srcStride + x * 4;
       if ( srcData[srcPos + faint::iA] == 0 ||
         ( srcData[ srcPos + faint::iA ] == maskColor.a &&
           srcData[ srcPos + faint::iR ] == maskColor.r &&
           srcData[ srcPos + faint::iG ] == maskColor.g &&
           srcData[ srcPos + faint::iB ] == maskColor.b ) ) {
         continue;
       }

      int dstPos = ( y + y0 ) * dstStride + ( x + x0 ) * 4;
      uchar alpha = srcData[srcPos + iA];
      dstData[dstPos + iR] = static_cast<uchar>((srcData[srcPos + iR] * alpha + dstData[dstPos + iR] * ( 255 - alpha )) / 255);
      dstData[dstPos + iG] = static_cast<uchar>((srcData[srcPos + iG] * alpha + dstData[dstPos + iG] * ( 255 - alpha ) ) / 255);
      dstData[dstPos + iB] = static_cast<uchar>((srcData[srcPos + iB] * alpha + dstData[dstPos + iB] * ( 255 - alpha )) / 255);
      dstData[dstPos + iA] = 255;
    }
  }
}

void blit( const Bitmap& src, DstBmp dst, const IntPoint& topLeft ){
  int x0 = topLeft.x;
  int y0 = topLeft.y;
  if ( !bmp_impl::inside( dst, src, x0, y0 ) ){
    return;
  }

  IntSize dstSz(dst.GetSize());
  const uint xMin = std::max( 0, -x0 );
  const uint yMin = std::max( 0, -y0 );
  const uint xMax = std::min( (int)dstSz.w - x0, (int)src.m_w );
  const uint yMax = std::min( (int)dstSz.h - y0, (int)src.m_h );
  uint dstStride = dst.GetStride();
  uint srcStride = src.GetStride();
  uchar* dstData = dst.GetRaw();
  const uchar* srcData = src.GetRaw();

  for ( uint y = yMin; y != yMax; y++ ){
    for ( uint x = xMin; x != xMax; x++ ){
      int srcPos = y * srcStride + x * 4;
      int dstPos = ( y + y0 ) * dstStride + ( x + x0 ) * 4;
      dstData[ dstPos ] = srcData[srcPos ];
      dstData[ dstPos + 1 ] = srcData[srcPos + 1 ];
      dstData[ dstPos + 2 ] = srcData[srcPos + 2 ];
      dstData[ dstPos + 3 ] = srcData[srcPos + 3 ];
    }
  }
}

void blit_masked( const Bitmap& src, DstBmp dst, const Color& maskColor, const IntPoint& topLeft ){
  int x0 = topLeft.x;
  int y0 = topLeft.y;
  if ( !bmp_impl::inside( dst, src, x0, y0 ) ){
    return;
  }
  const unsigned int xMin = std::max( 0, -x0 );
  const unsigned int yMin = std::max( 0, -y0 );

  IntSize dstSz(dst.GetSize());
  const unsigned int xMax = std::min( (int) dstSz.w - x0, (int)src.m_w );
  const unsigned int yMax = std::min( (int)dstSz.h - y0, (int)src.m_h  );

  const uint srcStride = src.GetStride();
  const uint dstStride = dst.GetStride();

  uchar* dstData = dst.GetRaw();
  const uchar* srcData = src.GetRaw();

  for ( uint y = yMin; y != yMax; y++ ){
    for ( uint x = xMin; x != xMax; x++ ){
      int srcPos = y * srcStride + x * 4;
      int dstPos = ( y + y0 ) * dstStride + ( x + x0 ) * 4;
      if (
          srcData[ srcPos + 3 ] != maskColor.a ||
          srcData[ srcPos + 2 ] != maskColor.r ||
          srcData[ srcPos + 1 ] != maskColor.g ||
          srcData[ srcPos ] != maskColor.b )
        {
          dstData[ dstPos ] = srcData[srcPos ];
          dstData[ dstPos + 1 ] = srcData[srcPos + 1 ];
          dstData[ dstPos + 2 ] = srcData[srcPos + 2 ];
          dstData[ dstPos + 3 ] = srcData[srcPos + 3 ];
        }
    }
  }
}

Brush circle_brush( uint w ){
  if ( w <= 1 ){
    return Brush( 1, 1, true );
  }

  uint rd = static_cast<uint>( w / 2.0 );

  int ofs = w % 2 == 0 ? 1 : 0;

  int f = 1 - rd;
  int ddF_x = 1;
  int ddF_y = -2 * rd;
  uint x = 0;
  uint y = rd;

  Brush b( w, w, false );

  int cx = rd;
  int cy = rd;
  b.Set( cx, cy + rd - ofs, 255 );
  b.Set( cx, cy - rd, 255 );
  b.Set( cx + rd - ofs, cy, 255 );
  b.Set( cx - rd, cy, 255);

  while(x <= y) {
    if( f >= 0 ) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    for ( size_t i = 0; i <= x; i++ ){
      b.Set( cx + i - ofs, cy + y - ofs, 255 );
      b.Set( cx - i, cy + y - ofs, 255 );
      b.Set( cx + i - ofs, cy - y, 255 );
      b.Set( cx - i, cy - y, 255 );
    }

    for ( size_t i = 0; i <= y; i++ ){
      b.Set( cx + i - ofs, cy + x - ofs, 255 );
      b.Set( cx - i, cy + x - ofs, 255 );
      b.Set( cx + i - ofs, cy - x, 255 );
      b.Set( cx - i, cy - x, 255 );
    }
  }
  for ( size_t i = 0; i != rd; i++ ){
    b.Set( cx - i, cy, 255 );
    b.Set( cx + i - ofs, cy, 255 );
  }
  return b;
}

void clear( Bitmap& bmp, const Color& c ){
  uchar* data = bmp.m_data;
  for ( int y = 0; y != bmp.m_h; y++ ){
    for ( int x = 0; x != bmp.m_w; x++ ){
      int dst = y * bmp.m_row_stride + x * 4;
      data[ dst + iR ] = c.r;
      data[ dst + iG ] = c.g;
      data[ dst + iB ] = c.b;
      data[ dst + iA ] = c.a;
    }
  }
}

void clear( Bitmap& bmp, const ColRGB& c ){
  clear( bmp, Color(c, 255) );
}

void clear( Bitmap& bmp, const DrawSource& src ){
  if ( src.IsColor() ){
    clear(bmp, src.GetColor());
  }
  else if ( src.IsPattern() ){
    clear_pattern(bmp, src.GetPattern());
  }
  else if ( src.IsGradient() ){
    // Fixme: Todo
  }
  else{
    assert(false);
  }
}

size_t count_colors( const Bitmap& bmp ){
  return get_palette(bmp).size();
}

void draw_ellipse( Bitmap& bmp, const IntRect& r, int lineWidth, const DrawSource& src, bool dashes ){
  DISPATCH( draw_ellipse_f,
    ColorFromColor(src.GetColor()),
    ColorFromPattern(src.GetPattern()),
    ColorFromGradient(src.GetGradient(), r),
    r, lineWidth, dashes );
}

void draw_line( Bitmap& bmp, const IntPoint& p0, const IntPoint& p1, const DrawSource& src, int lineWidth, bool dashes, LineCap cap ){
  DISPATCH( draw_line_f,
    ColorFromColor(src.GetColor()),
    ColorFromPattern(src.GetPattern()),
    ColorFromGradient(src.GetGradient(), inflated(bounding_rect(p0,p1), lineWidth)), // Fixme: This should be aligned with the line
    p0, p1, lineWidth, dashes, cap );
}

void draw_line_color( Bitmap& bmp, const IntPoint& p0, const IntPoint& p1, const Color& c, int lineWidth, bool dashes, LineCap cap ){
  draw_line_f( bmp, ColorFromColor(c), p0, p1, lineWidth, dashes, cap );
}

void draw_polygon( Bitmap& bmp, const std::vector<IntPoint>& points, const DrawSource& src, int lineWidth, bool dashes ){
  DISPATCH( draw_polygon_f,
    ColorFromColor(src.GetColor()),
    ColorFromPattern(src.GetPattern()),
    ColorFromGradient(src.GetGradient(), inflated(bounding_rect(points), lineWidth)),
    points, lineWidth, dashes );
}

void draw_polyline( Bitmap& bmp, const std::vector<IntPoint>& points, const DrawSource& src, int lineWidth, bool dashes, LineCap cap ){
  DISPATCH( draw_polyline_f,
    ColorFromColor(src.GetColor()),
    ColorFromPattern(src.GetPattern()),
    ColorFromGradient(src.GetGradient(), inflated(bounding_rect(points), lineWidth)),
    points, lineWidth, dashes, cap );
}

void draw_rect( Bitmap& bmp, const IntRect& r, const DrawSource& src, int lineWidth, bool dashes ){
  DISPATCH( draw_rect_f,
    ColorFromColor(src.GetColor()),
    ColorFromPattern(src.GetPattern()),
    ColorFromGradient(src.GetGradient(), r),
    r, lineWidth, dashes );
}

void draw_rect_color( Bitmap& bmp, const IntRect& r, const Color& c, int lineWidth, bool dashes ){
  draw_rect_f( bmp, ColorFromColor(c), r, lineWidth, dashes );
}

void erase_but( Bitmap& bmp, const Color& keep, const DrawSource& src ){
  auto is_not_color = [&bmp, &keep](int x, int y){
    return get_color_raw(bmp, x, y) != keep;
  };
  DISPATCH( set_pixels_if,
    ColorFromColor(src.GetColor()),
    ColorFromPattern(src.GetPattern()),
    ColorFromGradient(src.GetGradient(), IntRect(IntPoint(0,0),bmp.GetSize())),
    is_not_color );
}

void fill_ellipse( Bitmap& bmp, const IntRect& r, const DrawSource& src ){
  DISPATCH( fill_ellipse_f,
    ColorFromColor(src.GetColor()),
    ColorFromPattern(src.GetPattern()),
    ColorFromGradient(src.GetGradient(), r),
    r );
}

void fill_ellipse_color( Bitmap& bmp, const IntRect& r, const Color& c ){
  fill_ellipse_f( bmp, ColorFromColor(c), r );
}

void fill_polygon( Bitmap& bmp, const std::vector<IntPoint>& points, const DrawSource& src ){
  DISPATCH( fill_polygon_f,
    ColorFromColor(src.GetColor()),
    ColorFromPattern(src.GetPattern()),
    ColorFromGradient(src.GetGradient(), bounding_rect(points)),
    points );
}

void fill_rect( Bitmap& bmp, const IntRect& r, const DrawSource& src ){
  DISPATCH( fill_rect_f,
    ColorFromColor(src.GetColor()), ColorFromPattern(src.GetPattern()), ColorFromGradient(src.GetGradient(), r),
    r);
}

void fill_rect_color( Bitmap& bmp, const IntRect& r, const Color& c ){
  fill_rect_f(bmp, ColorFromColor(c), r);
}

void fill_rect_rgb( Bitmap& bmp, const IntRect& r, const ColRGB& c ){
  fill_rect_f(bmp, ColorFromColor(Color(c,255)), r );
}

void fill_triangle( Bitmap& bmp, const Point& p0, const Point& p1, const Point& p2, const DrawSource& src ){
  DISPATCH(fill_triangle_f,
    ColorFromColor(src.GetColor()),
    ColorFromPattern(src.GetPattern()),
    ColorFromGradient(src.GetGradient(), floored(bounding_rect(p0,p1,p2))), // Fixme: When used for arrowheads, this will use a different gradient offset than that of the line
    p0,p1,p2);
}

void fill_triangle_color( Bitmap& bmp, const Point& p0, const Point& p1, const Point& p2, const Color& c ){
  fill_triangle_f( bmp, ColorFromColor(c), p0, p1, p2 );
}

Bitmap flip( const Bitmap& bmp, Axis axis ){
  return axis == Axis::HORIZONTAL ?
    flip_horizontal(bmp) :
    flip_vertical(bmp);
}

void flood_fill( Bitmap& bmp, const IntPoint& pos, const Color& fillColor ){
  std::vector< IntPoint > v;
  const Color targetColor = get_color( bmp, pos );
  if ( targetColor == fillColor ){
    return;
  }

  v.push_back( pos );
  for ( uint i = 0; i != v.size(); i++ ){
    if ( get_color( bmp, v[i] ) == targetColor ){
      IntPoint w = v[i];
      IntPoint e = v[i];
      for ( EVER ){
        put_pixel( bmp, w, fillColor );
        if ( w.x - 1 < 0 || get_color_raw( bmp, w.x - 1, w.y ) != targetColor ){
          break;
        }
        w.x -= 1;
        if ( w.y != 0 && get_color_raw( bmp, w.x, w.y - 1 ) == targetColor ){
          v.push_back( IntPoint( w.x, w.y - 1 ) );
        }
        if ( w.y != bmp.m_h - 1 && get_color_raw( bmp, w.x, w.y + 1 ) == targetColor ){
          v.push_back( IntPoint( w.x, w.y + 1 ) );
        }
      }

      for ( EVER ){
        put_pixel( bmp, e, fillColor ); // Fixme: Will put twice due to w
        if ( e.y != 0 && get_color_raw( bmp, e.x, e.y - 1 ) == targetColor ){
          v.push_back( IntPoint( e.x, e.y - 1 ) );
        }
        if ( e.y != bmp.m_h - 1 && get_color_raw( bmp, e.x, e.y + 1 ) == targetColor ){
          v.push_back( IntPoint( e.x, e.y + 1 ) );
        }

        if ( e.x + 1 == bmp.m_w || get_color_raw( bmp, e.x + 1, e.y ) != targetColor ){
          break;
        }
        e.x += 1;
      }
    }
  }
}

void flood_fill_pattern_relative( Bitmap& bmp, const IntPoint& pos, const Pattern& pattern ){
  // Fixme: Use ColorFromPattern
  Bitmap filled(bmp);
  std::vector< IntPoint > v;
  const Color targetColor = get_color( bmp, pos );
  Color c = color_black();
  if ( c == targetColor ){
    c = color_white();
  }

  const Bitmap& patBmp(pattern.GetBitmap());
  const IntPoint patOffset = pattern.GetAnchor();

  v.push_back( pos );
  for ( uint i = 0; i != v.size(); i++ ){
    if ( get_color( bmp, v[i] ) == targetColor ){
      IntPoint w = v[i];
      IntPoint e = v[i];
      for ( EVER ){
        put_pixel( bmp, w, c );
        put_pixel( filled, w, get_color_modulo( patBmp, w + patOffset ));
        if ( w.x - 1 < 0 || get_color_raw( bmp, w.x - 1, w.y ) != targetColor ){
          break;
        }
        w.x -= 1;
        if ( w.y != 0 && get_color_raw( bmp, w.x, w.y - 1 ) == targetColor ){
          v.push_back( IntPoint( w.x, w.y - 1 ) );
        }
        if ( w.y != bmp.m_h - 1 && get_color_raw( bmp, w.x, w.y + 1 ) == targetColor ){
          v.push_back( IntPoint( w.x, w.y + 1 ) );
        }
      }

      for ( EVER ){
        put_pixel( bmp, e, c );
        put_pixel( filled, e, get_color_modulo( patBmp, ( e + patOffset ) ) );
        if ( e.y != 0 && get_color_raw( bmp, e.x, e.y - 1 ) == targetColor ){
          v.push_back( IntPoint( e.x, e.y - 1 ) );
        }
        if ( e.y != bmp.m_h - 1 && get_color_raw( bmp, e.x, e.y + 1 ) == targetColor ){
          v.push_back( IntPoint( e.x, e.y + 1 ) );
        }

        if ( e.x + 1 == bmp.m_w || get_color_raw( bmp, e.x + 1, e.y ) != targetColor ){
          break;
        }
        e.x += 1;
      }
    }
  }
  blit( filled, onto(bmp), IntPoint(0,0) );
}

void flood_fill2( Bitmap& bmp, const IntPoint& pos, const DrawSource& src ){ // Fixme
  if ( src.IsColor() ){
    flood_fill( bmp, pos, src.GetColor() );
  }
  else if ( src.IsPattern() ){
    flood_fill_pattern_relative( bmp, pos, src.GetPattern() );
  }
  else if ( src.IsGradient() ){
    // Fixme: Todo
  }
  else {
    assert(false);
  }
}

Color get_color( const Bitmap& bmp, const IntPoint& pos ){
  return get_color_raw( bmp, pos.x, pos.y );
}

Color get_color_raw( const Bitmap& bmp, int x, int y ){
  int pos = y * bmp.m_row_stride + x * 4;
  uchar* data = bmp.m_data;
  return Color( data[ pos + iR ], data[pos + iG ], data[ pos + iB ], data[ pos + iA ] );
}

Bitmap get_null_bitmap(){
  Bitmap nullBitmap = Bitmap();
  return nullBitmap;
}

std::vector<Color> get_palette(const Bitmap& bmp){
  std::set<Color> colors;
  for ( int y = 0; y != bmp.m_h; y++ ){
    for ( int x = 0; x != bmp.m_w; x++ ){
      colors.insert(get_color_raw(bmp, x, y));
    }
  }
  return std::vector<Color>(colors.begin(), colors.end());
}

bool inside( const IntRect& r, const Bitmap& bmp ){
  if ( r.x < 0 || r.y < 0 ){
    return false;
  }
  if ( r.x + r.w > bmp.m_w || r.y + r.h > bmp.m_h ){
    return false;
  }
  return true;
}

bool is_blank( const Bitmap& bmp ){
  Color color = get_color( bmp, IntPoint(0,0) );
  for ( int y = 0; y != bmp.m_h; y++ ){
    for ( int x = 0; x != bmp.m_w; x++ ){
      if ( get_color( bmp, IntPoint(x,y) ) != color ){
        return false;
      }
    }
  }
  return true;
}

bool point_in_bitmap( const Bitmap& bmp, const IntPoint& pos ){
  return pos.x >= 0 && pos.y >= 0 && pos.x < bmp.m_w && pos.y < bmp.m_h;
}

void put_pixel( Bitmap& bmp, const IntPoint& pos, const Color& color ){
  put_pixel_raw(bmp, pos.x, pos.y, color);
}

void put_pixel_raw( Bitmap& bmp, int x, int y, const Color& color ){
  if ( x < 0 || y < 0 || x >= bmp.m_w || y >= bmp.m_h ) {
    return;
  }
  uchar* data = bmp.GetRaw();
  int pos = y * bmp.m_row_stride + x * 4;
  data[pos + iA] = color.a;
  data[pos + iR] = color.r;
  data[pos + iG] = color.g;
  data[pos + iB] = color.b;
}

void replace_color_color( Bitmap& bmp, const OldColor& in_oldColor, const NewColor& in_newColor ){
  const Color& oldColor(in_oldColor.Get());
  const Color& newColor(in_newColor.Get());
  for ( int y = 0; y != bmp.m_h; y++ ){
    uchar* row = bmp.m_data + y * bmp.m_row_stride;
    for ( int x = 0; x != bmp.m_w * 4; x += 4 ){
      color_ptr current(row + x);
      if ( current == oldColor ){
        current.Set(newColor);
      }
    }
  }
}

void replace_color_pattern( Bitmap& bmp, const OldColor& in_oldColor, const Pattern& pattern ){
  const Color& oldColor(in_oldColor.Get());
  const Bitmap& patBmp(pattern.GetBitmap());
  const IntPoint patOffset(pattern.GetAnchor());
  const IntSize sz(bmp.GetSize());
  for ( int y = 0; y != sz.h; y++ ){
    for ( int x = 0; x != sz.w; x++ ){
      if ( get_color_raw(bmp, x,y) == oldColor ){
        put_pixel_raw( bmp, x, y, get_color_modulo(patBmp, IntPoint(x, y) + patOffset) );
      }
    }
  }
}

void replace_color( Bitmap& bmp, const OldColor& in_oldColor, const DrawSource& src ){
  if ( src.IsColor() ){
    replace_color_color( bmp, in_oldColor, New(src.GetColor()) );
  }
  else if ( src.IsPattern() ){
    replace_color_pattern( bmp, in_oldColor, src.GetPattern() );
  }
  else if ( src.IsGradient() ){
    // Fixme: Todo
  }
  else {
    assert( false ); // Unknown DrawSource type
  }
}

// Note: rotate is implemented in rotate.cpp

Bitmap rotate_90cw( const Bitmap& src ){
  Bitmap dst(transposed( src.GetSize()));
  uchar* pDst = dst.m_data;
  uchar* pSrc = src.m_data;
  for ( int y = 0; y != src.m_h; y++ ){
    for ( int x = 0 ; x != src.m_w; x++ ){
      int iSrc = y * src.m_row_stride + x * 4;
      int iDst = x * dst.m_row_stride + ( dst.m_w - y - 1) * 4;
      pDst[ iDst + iR ] = pSrc[iSrc + iR ];
      pDst[ iDst + iG ] = pSrc[iSrc + iG ];
      pDst[ iDst + iB ] = pSrc[iSrc + iB ];
      pDst[ iDst + iA ] = pSrc[iSrc + iA ];
    }
  }
  return dst;
}

Bitmap scale(const Bitmap& bmp, const Scale& scale, ScaleQuality quality){
  switch ( quality ){
  case ScaleQuality::NEAREST:
    return scale_nearest(bmp, scale);
  case ScaleQuality::BILINEAR:
    return scale_bilinear(bmp, scale);
  };
  assert( false );
  return Bitmap();
}

Bitmap scale_bilinear( const Bitmap& src, const Scale& scale ){
  IntSize newSize(constrained(truncated(floated(src.GetSize()) * abs(scale)),
      min_t(1), min_t(1)));
  if ( newSize == src.GetSize() ){
    return Bitmap(src);
  }

  Bitmap dst(newSize);
  const coord x_ratio = floated(src.m_w - 1) / floated(newSize.w);
  const coord y_ratio = floated(src.m_h - 1) / floated(newSize.h);
  const uchar* data = src.m_data;

  for ( int yDst = 0; yDst != newSize.h; yDst++ ){
    for ( int xDst = 0; xDst != newSize.w; xDst++ ){
      int xSrc = truncated(x_ratio * xDst);
      int ySrc = truncated(y_ratio * yDst);
      const coord x_diff = x_ratio * xDst - xSrc;
      const coord y_diff = y_ratio * yDst - ySrc;
      const int srcIndex = ySrc * src.m_row_stride + xSrc * 4;
      const_color_ptr a(data + srcIndex);
      const_color_ptr b(data + srcIndex + 4);
      const_color_ptr c(data + srcIndex + src.m_row_stride);
      const_color_ptr d(data + srcIndex + src.m_row_stride + 4);

      // Fixme: Check for possible overflow
      const uchar blue = static_cast<uchar>((a.b)*(1-x_diff)*(1-y_diff) + (b.b)*(x_diff)*(1-y_diff) +
        (c.b)*(y_diff)*(1-x_diff) + (d.b)*(x_diff*y_diff) + 0.5);

      const uchar green = static_cast<uchar>( a.g*(1-x_diff)*(1-y_diff) + b.g*(x_diff)*(1-y_diff) +
        (c.g)*(y_diff)*(1-x_diff) + (d.g)*(x_diff*y_diff) + 0.5 );

      const uchar red = static_cast<uchar>( a.r * (1-x_diff)*(1-y_diff) + b.r*(x_diff)*(1-y_diff) +
        c.r*(y_diff)*(1-x_diff) + (d.r)*(x_diff*y_diff) + 0.5 );

      const uchar alpha = static_cast<uchar>( a.a * (1-x_diff)*(1-y_diff) + b.a*(x_diff)*(1-y_diff) +
        c.a*(y_diff)*(1-x_diff) + (d.a)*(x_diff*y_diff) + 0.5 );

      uchar* rDst = dst.m_data + yDst * ( dst.m_row_stride ) + xDst * 4;
      *( rDst + iR ) = red;
      *( rDst + iG ) = green;
      *( rDst + iB ) = blue;
      *( rDst + iA ) = alpha;
    }
  }

  if ( scale.x < 0 ){
    dst = flip_horizontal(dst);
  }
  if ( scale.y < 0 ) {
    dst = flip_vertical(dst);
  }
  return dst;
}

Bitmap scale_nearest( const Bitmap& src, int scale ){
  const int w2 = src.m_w * scale;
  const int h2 = src.m_h * scale;

  Bitmap scaled(IntSize(w2, h2));
  int x_ratio = (int)(( src.m_w << 16)/ scaled.m_w) + 1;
  int y_ratio = (int)(( src.m_h << 16)/ scaled.m_h) + 1;
  int x2, y2 ;
  uchar* p_dst = scaled.m_data;
  const uchar* p_src = src.m_data;

  for (int i = 0; i< h2; i++){
    for (int j = 0; j < w2; j++){
      x2 = ( (j*x_ratio)>>16 );
      y2 = ( (i*y_ratio)>>16 );

      uchar* rDst = p_dst + i * ( scaled.m_row_stride ) + j * 4;
      const uchar* rSrc = p_src + y2 * ( src.m_row_stride ) + x2 * 4;

      *( rDst + 0 ) = *rSrc;
      *( rDst + 1 ) = *(rSrc + 1);
      *( rDst + 2 ) = *(rSrc + 2);
      *( rDst + 3 ) = *(rSrc + 3);
    }
  }
  return scaled;
}

Bitmap scale_nearest( const Bitmap& src, const Scale& scale ){
  const int w2 = static_cast<int>( src.m_w * scale.x );
  const int h2 = static_cast<int>( src.m_h * scale.y );

  Bitmap scaled(IntSize(w2, h2));
  int x_ratio = (int)(( src.m_w << 16)/ scaled.m_w) + 1;
  int y_ratio = (int)(( src.m_h << 16)/ scaled.m_h) + 1;
  int x2, y2 ;
  uchar* p_dst = scaled.m_data;
  const uchar* p_src = src.m_data;

  for (int i = 0; i< h2; i++){
    for (int j = 0; j < w2; j++){
      x2 = ( (j*x_ratio)>>16 );
      y2 = ( (i*y_ratio)>>16 );

      uchar* rDst = p_dst + i * ( scaled.m_row_stride ) + j * 4;
      const uchar* rSrc = p_src + y2 * ( src.m_row_stride ) + x2 * 4;

      *( rDst + 0 ) = *rSrc;
      *( rDst + 1 ) = *(rSrc + 1);
      *( rDst + 2 ) = *(rSrc + 2);
      *( rDst + 3 ) = *(rSrc + 3);
    }
  }
  return scaled;
}

Bitmap scaled_subbitmap( const Bitmap& src, const Scale& scale, const IntRect& r ){
  assert(r.x >= 0 );
  assert(r.y >= 0 );
  IntSize newSize( rounded(r.w * scale.x), rounded(r.h * scale.y));
  newSize.w = std::max(newSize.w, 1);
  newSize.h = std::max(newSize.h, 1);
  Bitmap scaled(newSize);
  coord x_ratio = 1.0 / scale.x;
  coord y_ratio = 1.0 / scale.y;

  const uchar* data = src.m_data;
  for ( int i = 0; i != newSize.h; i++ ){
    for ( int j = 0; j != newSize.w; j++ ){
      int x = truncated( x_ratio * j + r.x );
      int y = truncated( y_ratio * i + r.y );
      coord x_diff = ( x_ratio * j + r.x ) - x;
      coord y_diff = ( y_ratio * i + r.y ) - y;
      int index = y * src.m_row_stride + x * 4;

      const_color_ptr a( data + index);
      const_color_ptr b( data + index + 4 );
      const_color_ptr c( data + index + src.m_row_stride );
      const_color_ptr d( data + index + src.m_row_stride + 4 );

      uchar blue = static_cast<uchar>((a.b)*(1-x_diff)*(1-y_diff) + (b.b)*(x_diff)*(1-y_diff) +
        (c.b)*(y_diff)*(1-x_diff) + (d.b)*(x_diff*y_diff));

      uchar green = static_cast<uchar>( a.g*(1-x_diff)*(1-y_diff) + b.g*(x_diff)*(1-y_diff) +
        (c.g)*(y_diff)*(1-x_diff) + (d.g)*(x_diff*y_diff) );

      uchar red = static_cast<uchar>( a.r * (1-x_diff)*(1-y_diff) + b.r*(x_diff)*(1-y_diff) +
        c.r*(y_diff)*(1-x_diff) + (d.r)*(x_diff*y_diff) );

      uchar alpha = static_cast<uchar>( a.a * (1-x_diff)*(1-y_diff) + b.a*(x_diff)*(1-y_diff) +
        c.a*(y_diff)*(1-x_diff) + (d.a)*(x_diff*y_diff) );

      uchar* rDst = scaled.m_data + i * ( scaled.m_row_stride ) + j * 4;

      *( rDst + iR ) = red;
      *( rDst + iG ) = green;
      *( rDst + iB ) = blue;
      *( rDst + iA ) = alpha;
    }
  }
  return scaled;
}

void set_alpha( Bitmap& bmp, uchar a ){
  uchar* data = bmp.GetRaw();
  const uint stride = bmp.GetStride();
  IntSize sz(bmp.GetSize());
  for ( int y = 0; y != sz.h; y++ ){
    for ( int x = 0; x != sz.w; x++ ){
      int i = y * stride + x * 4;
      data[i + iA] = a;
    }
  }
}

void stroke( Bitmap& bmp, const IntPoint& p0, const IntPoint& p1, const Brush& b, const Color& c ){
  int x0 = p0.x;
  int y0 = p0.y;
  int x1 = p1.x;
  int y1 = p1.y;
  const bool steep = int_abs( y1 - y0 ) > int_abs( x1 - x0 );
  if ( steep ){
    swap( x0, y0 );
    swap( x1, y1 );
  }
  if ( x0 > x1 ){
    swap( x0, x1 );
    swap( y0, y1 );
  }

  int dx = x1 - x0;
  int dy = int_abs(y1 - y0);
  int err = 0;
  int y = y0;
  int yStep = y0 < y1 ? 1 : -1;
  for ( int x = x0; x<= x1; x++ ){
    err += dy;
    if ( steep ){
      brush_stroke( bmp, y, x, b, c );
    }
    else {
      brush_stroke( bmp, x, y, b, c );
    }

    if ( 2 * err > dx ){
      y += yStep;
      err -= dx;
    }
  }
}

void stroke( Bitmap& bmp, const std::vector<IntPoint>& points, const Brush& b, const Color& c ){
  if ( points.size() == 1 ){
    const IntPoint p(points.back());
    stroke( bmp, p, p, b, c );
    return;
  }

  IntPoint prev(points[0]);
  for ( size_t i = 1; i != points.size(); i++ ){
    stroke( bmp, prev, points[i], b, c );
    prev = points[i];
  }
}

static void stroke_pattern( Bitmap& bmp, const std::vector<IntPoint>& points, const Brush& b, const Pattern& pattern ){
  Bitmap strokeMap(bmp.GetSize(), color_white());
  stroke(strokeMap, points, b, color_black() );
  set_pixels_if( bmp, ColorFromPattern(pattern),
    IsColor(strokeMap, color_black()) );
}

void stroke( Bitmap& bmp, const std::vector<IntPoint>& points, const Brush& b, const DrawSource& src ){
  if ( src.IsColor() ){
    stroke( bmp, points, b, src.GetColor() );
  }
  else if ( src.IsPattern() ){
    stroke_pattern( bmp, points, b, src.GetPattern() );
  }
  else if ( src.IsGradient() ){
    // Fixme: todo
  }
  else {
    assert( false ); // Unknown DrawSource type
  }
}

Bitmap sub_bitmap( const Bitmap& orig, const IntRect& r ){
  uint origStride = orig.m_row_stride;
  const uchar* origData = orig.m_data;

  Bitmap bmp( r.GetSize() );
  uchar* data = bmp.GetRaw();
  uint destStride = bmp.m_row_stride;

  for ( int y = 0; y != r.h; y++ ){
    for ( int x = 0; x != r.w; x++ ){
      int dstPos = y * destStride + x * 4;
      int srcPos = ( y + r.y ) * origStride + ( x + r.x ) * 4;
      data[ dstPos ] = origData[ srcPos ];
      data[ dstPos + 1 ] = origData[ srcPos + 1 ];
      data[ dstPos + 2 ] = origData[ srcPos + 2 ];
      data[ dstPos + 3 ] = origData[ srcPos + 3 ];
    }
  }
  return bmp;
}

} // namespace
