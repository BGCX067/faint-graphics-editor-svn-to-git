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
#include "bitmap.hh"
#include "geo/geotypes.hh"
#include "rendering/cairocontext.hh"

using std::swap;
using std::abs;

#define EVER ;;
namespace faint{

const uint bpp = 4;
const int iR = 2;
const int iG = 1;
const int iB = 0;
const int iA = 3;

Bitmap::Bitmap( BitmapFormat format, const IntSize& sz, uint stride ):
  m_format( format ),
  m_row_stride( stride ),
  m_w( static_cast<uint>(sz.w) ),
  m_h( static_cast<uint>(sz.h) ),
  m_data(0)
{
  assert( sz.w > 0 );
  assert( sz.h > 0 );
  m_data = new uchar[ stride * m_h ];
  memset( m_data, 0, stride * m_h );
}

Bitmap::Bitmap( const Bitmap& other ) :
  m_format( other.m_format ),
  m_row_stride( other.m_row_stride ),
  m_w( other.m_w ),
  m_h( other.m_h ),
  m_data(0)
{
  if ( other.m_data != 0 ){
    m_data = new uchar[ m_row_stride * m_h ];
    memcpy( m_data, other.m_data, m_row_stride * m_h );
  }
}

#ifdef FAINT_RVALUE_REFERENCES
Bitmap::Bitmap( Bitmap&& source ) :
  m_format( source.m_format ),
  m_row_stride( source.m_row_stride ),
  m_w( source.m_w ),
  m_h( source.m_h ),
  m_data( source.m_data )
{
  source.m_data = 0;
  source.m_w = 0;
  source.m_h = 0;
}
#endif
Bitmap::Bitmap() :
  m_format( INVALID_BMP_FORMAT ),
  m_row_stride(0)
{
  m_w = m_h = 0;
  m_data = 0;
}

Bitmap::~Bitmap(){
  delete[] m_data;
}

Bitmap& Bitmap::operator=( const Bitmap& other ){
  if ( &other == this ){
    return *this;
  }
  delete[] m_data;
  m_data = 0;
  m_format = other.m_format;
  m_w = other.m_w;
  m_h = other.m_h;
  m_row_stride = other.m_row_stride;
  if ( other.m_data != 0 ){
    m_data = new uchar[ m_row_stride * m_h ];
    memcpy( m_data, other.m_data, m_row_stride * m_h );
  }
  return *this;
}

#ifdef FAINT_RVALUE_REFERENCES
Bitmap& Bitmap::operator=( Bitmap&& other ){
  assert(&other != this);
  delete[] m_data;
  m_data = 0;

  m_format = other.m_format;
  m_row_stride = other.m_row_stride;
  m_w = other.m_w;
  m_h = other.m_h;

  m_data = other.m_data;
  other.m_data = 0;
  other.m_w = 0;
  other.m_h = 0;
  return *this;
}
#endif

Brush::Brush( uint w, uint h, bool fill ){
  m_w = w;
  m_h = h;
  m_data = new uchar[ w * h ];
  if ( fill ){
    memset( m_data, 255, w * h );
  }
  else {
    memset( m_data, 0, w * h );
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

void Brush::Set( uint x, uint y, uchar value ){
  m_data[ y * m_w + x ] = value;
}

uchar Brush::Get( uint x, uint y ) const{
  return m_data[ y * m_w + x ];
}

}

namespace bmp_impl{
using namespace faint;
struct Colors{
  uchar r;
  uchar g;
  uchar b;
  uchar a;
  void Set( const uchar* p ){
    r = *(p + iR);
    g = *(p + iG);
    b = *(p + iB);
    a = *(p + iA);
  }
};

int int_abs( int v ){
  return v < 0 ? -v : v;
}

void sort_y( faint::coord& x0, faint::coord& y0, faint::coord& x1, faint::coord& y1, faint::coord& x2, faint::coord& y2 ){
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

IntPoint get_offset( int x0, int y0, int x1, int y1, int lineWidth ){
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

  faint::coord distance = 0;
  faint::coord diag = sqrt(2.0);
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

inline bool inside( const DstBmp& dst, const faint::Bitmap& src, int x0, int y0 ){
  IntSize sz(dst.GetSize());
  if ( x0 >= static_cast<int>(sz.w) || y0 >= static_cast<int>(sz.h) ){
    return false;
  }
  if ( x0 + static_cast<int>(src.m_w) < 0 || y0 + static_cast<int>(src.m_h) < 0 ){
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

void scanline( faint::Bitmap& bmp, int x0, int x1, int y, const faint::Color& c ){
  if ( x0 > x1 ){
    swap(x0,x1);
  }
  for ( int x = x0; x <= x1; x++ ){
    put_pixel_raw( bmp, x, y, c );
  }
}

void scanline_fl( faint::Bitmap& bmp, faint::coord x0, faint::coord x1, faint::coord y, const faint::Color& c ){
  scanline( bmp, static_cast<int>(x0), static_cast<int>(x1), static_cast<int>(y), c );
}

void vertical( faint::Bitmap& bmp, int y0, int y1, int x, const faint::Color& c ){
  if ( y0 > y1 ){
    swap(y0,y1);
  }
  for ( int y = y0; y <= y1; y++ ){
    put_pixel_raw( bmp, x, y, c );
  }
}

void line_circle( Bitmap& bmp, const IntPoint& p, int lineWidth, const Color& c ){
  int halfLw = ( lineWidth - 1 ) / 2;
  const IntPoint e0( -halfLw, -halfLw );
  const IntPoint e1(halfLw, halfLw);
  fill_ellipse( bmp, IntRect(p + e0, p + e1), c );
}

void draw_wide_line( faint::Bitmap& bmp, const IntPoint& p0, const IntPoint& p1, const faint::Color& c, int lineWidth, bool dashes, LineCap::type cap ){
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
	  put_pixel_raw( bmp, at_y - yStep * pt.y / 2, at_x - yStep * pt.x / 2, c );
	}
      }
      else{
	if ( on ){
	  put_pixel_raw( bmp, at_x - yStep * pt.x / 2, at_y - yStep * pt.y / 2, c );
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
    line_circle(bmp, p0, lineWidth, c);
    line_circle(bmp, p1, lineWidth, c);
  }
}

inline void brush_stroke( Bitmap& bmp, int x, int y, const Brush& b, const Color& c ){
  int xCenter = b.m_w / 2;
  int yCenter = b.m_h / 2;
  for ( unsigned int yB = 0; yB != b.m_h; yB++ ){
    for ( unsigned int xB = 0; xB != b.m_w; xB++ ){
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

} // namespace bmp_impl
namespace faint{
using namespace bmp_impl;

Bitmap alpha_blended( const Bitmap& src, const Color& color ){
  faint::Bitmap dst( cairo_compatible_bitmap( src.GetSize() ) );
  clear( dst, color );
  blend( src, onto(dst), IntPoint(0,0) );
  return dst;
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

void blend_masked( const Bitmap& src, DstBmp dst, const faint::Color& maskColor, const IntPoint& topLeft ){
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
  for ( uint y = 0; y != bmp.m_h; y++ ){
    for ( uint x = 0; x != bmp.m_w; x++ ){
      int dst = y * bmp.m_row_stride + x * 4;
      data[ dst + iR ] = c.r;
      data[ dst + iG ] = c.g;
      data[ dst + iB ] = c.b;
      data[ dst + iA ] = c.a;
    }
  }
}

size_t count_colors( const Bitmap& bmp ){
  return get_palette(bmp).size();
}

void desaturate_simple( Bitmap& bmp ){
  uchar* data = bmp.m_data;
  for ( uint y = 0; y != bmp.m_h; y++ ){
    for ( uint x = 0 ; x != bmp.m_w; x++ ){
      int dst = y * bmp.m_row_stride + x * 4;
      uchar gray = static_cast<uchar>(( data[dst + iR ] + data[dst + iG ] + data[dst + iB ] ) / 3);
      data[dst + iR ] = gray;
      data[dst + iG ] = gray;
      data[dst + iB ] = gray;
    }
  }
}

void desaturate_weighted( Bitmap& bmp ){
  uchar* data = bmp.m_data;
  for ( uint y = 0; y != bmp.m_h; y++ ){
    for ( uint x = 0 ; x != bmp.m_w; x++ ){
      int dst = y * bmp.m_row_stride + x * 4;
      uchar gray = static_cast<uchar>(( 0.3 * data[dst + iR ] + 0.59 * data[dst + iG ] + 0.11 * data[dst + iB ] ));
      data[dst + iR ] = gray;
      data[dst + iG ] = gray;
      data[dst + iB ] = gray;
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

void blit_masked( const Bitmap& src, DstBmp dst, const faint::Color& maskColor, const IntPoint& topLeft ){
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

void draw_wide_ellipse( faint::Bitmap& bmp, const IntRect& r, int lineWidth, const faint::Color& color ){
  IntPoint radiuses(truncated(floated(IntPoint(r.w, r.h)) / LITCRD(2.0)));
  int x0 = r.x + radiuses.x;
  int y0 = r.y + radiuses.y;
  int a = radiuses.x;
  int b = radiuses.y;

  if ( a == 0 ){
    draw_line( bmp, r.TopLeft(), r.BottomLeft(), color, lineWidth, false, LineCap::BUTT );
    return;
  }
  if ( b == 0 ){
    draw_line( bmp, r.TopLeft(), r.TopRight(), color, lineWidth, false, LineCap::BUTT );
    return;
  }

  std::map<int,int> points = ellipse_points( x0, y0, a - lineWidth, b - lineWidth );
  for ( std::map<int,int>::iterator it = points.begin(); it != points.end(); ++it ){
    put_pixel_raw( bmp, it->second, it->first, color );
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
    put_pixel_raw( bmp, x0 + x - xoffset, y0 + y - yoffset, color );

    if ( points.find( y0 + y ) != points.end() ){
      int x1 = points[y0 + y];
      scanline( bmp, x0 - x, x1, y0 + y - yoffset, color );
      int dx = x1 - ( x0 - x );
      scanline( bmp, x0 + x - dx - xoffset, x0 + x - xoffset, y0 + y - yoffset, color );
    }
    else {
      scanline( bmp, x0 - x, x0 + x - xoffset, y0 + y - yoffset, color );
    }

    if ( points.find( y0 - y ) != points.end() ){
      int x1 = points[y0 - y];
      scanline( bmp, x0 - x, x1, y0 - y, color );
      int dx = x1 - ( x0 - x );
      scanline( bmp, x0 + x - xoffset - dx, x0 + x - xoffset, y0 - y, color);
    }
    else {
      scanline( bmp, x0 - x, x0 + x - xoffset, y0 - y, color );
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
      scanline( bmp, x0 - x, x1, y0 + y - yoffset, color );
      int dx = x1 - ( x0 - x );
      scanline( bmp, x0 + x - xoffset - dx, x0 + x - xoffset, y0 + y - yoffset, color );
    }
    else {
      scanline( bmp, x0 - x, x0 + x - xoffset, y0 + y - yoffset, color );
    }

    if ( points.find( y0 - y ) != points.end() ){
      int x1 = points[y0 - y];
      scanline( bmp, x0 - x, x1, y0 - y, color );
      int dx = x1 - ( x0 - x );
      scanline( bmp, x0 + x - xoffset - dx, x0 + x - xoffset, y0 - y, color );
    }
    else {
      scanline( bmp, x0 - x, x0 + x - xoffset, y0 - y, color );
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

void draw_ellipse( faint::Bitmap& bmp, const IntRect& r, int lineWidth, const faint::Color& c ){
  if ( lineWidth > 1 ){
    draw_wide_ellipse( bmp, r, lineWidth, c );
    return;
  }

  IntPoint radiuses(truncated(floated(IntPoint(r.w, r.h)) / LITCRD(2.0)));
  int x0 = r.x + radiuses.x;
  int y0 = r.y + radiuses.y;
  int a = radiuses.x;
  int b = radiuses.y;

  if ( a == 0 ){
    draw_line( bmp, r.TopLeft(), r.BottomLeft(), c, 1, false, LineCap::BUTT );
    return;
  }
  if ( b == 0 ){
    draw_line( bmp, r.TopLeft(), r.TopRight(), c, 1, false, LineCap::BUTT );
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
    put_pixel_raw( bmp, x0 + x - xoffset, y0 + y - yoffset, c );
    put_pixel_raw( bmp, x0 - x, y0 + y - yoffset, c );
    put_pixel_raw( bmp, x0 - x, y0 - y, c );
    put_pixel_raw( bmp, x0 + x - xoffset, y0 - y, c );
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
    put_pixel_raw( bmp, x0 + x - xoffset, y0 + y - yoffset, c);
    put_pixel_raw( bmp, x0 - x, y0 + y - yoffset, c);
    put_pixel_raw( bmp, x0 - x, y0 - y, c);
    put_pixel_raw( bmp, x0 + x - xoffset, y0 - y, c);
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

void draw_faint_circle( Bitmap& bmp, const IntPoint& center, int radius){
  int x0 = center.x;
  int y0 = center.y;
  int f = 1 - radius;
  int ddF_x = 1;
  int ddF_y = -2 * radius;
  int x = 0;
  int y = radius;
  faint::Color c(0,0,0);
  put_pixel_raw( bmp, x0, y0 + radius, c);
  put_pixel_raw( bmp, x0, y0 - radius, c);
  put_pixel_raw( bmp, x0 + radius, y0, c);
  put_pixel_raw( bmp, x0 - radius, y0, c);

  while( x < y ) {
    if( f >= 0 ) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    put_pixel_raw( bmp, x0 + x, y0 + y, c );
    put_pixel_raw( bmp, x0 - x, y0 + y, c );
    put_pixel_raw( bmp, x0 + x, y0 - y, c );
    put_pixel_raw( bmp, x0 - x, y0 - y, c );
    put_pixel_raw( bmp, x0 + y, y0 + x, c );
    put_pixel_raw( bmp, x0 - y, y0 + x, c );
    put_pixel_raw( bmp, x0 + y, y0 - x, c );
    put_pixel_raw( bmp, x0 - y, y0 - x, c );
  }
}

  void draw_line( Bitmap& bmp, const IntPoint& p0, const IntPoint& p1, const Color& c, int lineWidth, bool dashes, LineCap::type cap ){
  lineWidth = std::max(1, lineWidth);
  if ( lineWidth > 1 ){
    draw_wide_line( bmp, p0, p1, c, lineWidth, dashes, cap );
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
  faint::coord err = 0;
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
	put_pixel_raw( bmp, y, x, c );
      }
    }
    else {
      if ( on ){
	put_pixel_raw( bmp, x, y, c );
      }
    }
    if ( 2 * err > dx ){
      y += yStep;
      err -= dx;
    }
  }
}

void draw_polygon( Bitmap& bmp, const std::vector<IntPoint>& points, const Color& c, int lineWidth, bool dashes ){
  if ( points.empty() ){
    return;
  }
  if ( points.size() == 1 ){
    const IntPoint& p(points[0]);
    draw_line( bmp, p, p, c, lineWidth, dashes, LineCap::ROUND );
  }

  for ( size_t i = 1; i != points.size(); i++ ){
    draw_line( bmp, points[ i - 1], points[i], c, lineWidth, dashes, LineCap::BUTT );
    line_circle( bmp, points[i], lineWidth, c );
  }
  draw_line( bmp, points.back(), points[0], c, lineWidth, dashes, LineCap::BUTT );
  line_circle( bmp, points[0], lineWidth, c );

}

void draw_polyline( Bitmap& bmp, const std::vector<IntPoint>& points, const Color& c, int lineWidth, bool dashes, LineCap::type cap ){
  if ( points.empty() ){
    return;
  }
  if ( points.size() == 1 ){
    const IntPoint& p(points[0]);
    draw_line( bmp, p, p, c, lineWidth, dashes, cap );
  }

  for ( size_t i = 1; i != points.size(); i++ ){
    draw_line( bmp, points[ i - 1], points[i], c, lineWidth, dashes, LineCap::BUTT );
    if ( i != points.size() - 1 ){
      line_circle( bmp, points[i], lineWidth, c );
    }
  }
  if ( cap == LineCap::ROUND ){
    line_circle( bmp, points[0], lineWidth, c );
    line_circle( bmp, points.back(), lineWidth, c );
  }
}

void draw_rect( Bitmap& bmp, const IntRect& r, const faint::Color& c, int lineWidth, bool dashes ){
  if ( !dashes ){
    lineWidth = std::max(1, lineWidth);
    for ( int i = 0; i != lineWidth; i++ ){
      scanline( bmp, r.Left(), r.Right(), r.Top() + i, c );
      scanline( bmp, r.Left(), r.Right(), r.Bottom() - i, c );
      vertical( bmp, r.Top(), r.Bottom(), r.Left() + i, c );
      vertical( bmp, r.Top(), r.Bottom(), r.Right() - i, c );
    }
  }
  else {
    draw_polyline( bmp, as_line_path(r), c, lineWidth, true, LineCap::BUTT );
  }
}

void erase_but( faint::Bitmap& bmp, const faint::Color& keep, const faint::Color& erase ){
  for ( size_t y = 0; y != bmp.m_h; y++ ){
    uchar* data = bmp.m_data + y * bmp.m_row_stride;
    for ( size_t x = 0; x != bmp.m_w * 4; x += 4 ){ // Fixme: bpp-constant
      uchar* pos = data + x;
      uchar* r = pos +iR;
      uchar* g = pos + iG;
      uchar* b = pos + iB;
      uchar* a = pos + iA;
      if ( *r != keep.r || *g != keep.g || *b != keep.b || *a != keep.a ){
	*r = erase.r;
	*g = erase.g;
	*b = erase.b;
	*a = erase.a;
      }
    }
  }
}

void fill_ellipse( faint::Bitmap& bmp, const IntRect& r, const faint::Color& c ){
  IntPoint radiuses(truncated(floated(IntPoint(r.w, r.h)) / LITCRD(2.0)));
  int x0 = r.x + radiuses.x;
  int y0 = r.y + radiuses.y;
  int a = radiuses.x;
  int b = radiuses.y;

  if ( a == 0 ){
    vertical( bmp, r.Top(), r.Bottom(), r.Left(), c);
    return;
  }
  if ( b == 0 ){
    scanline( bmp, r.Left(), r.Right(), r.Top(), c );
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
    scanline( bmp, x0 - x, x0 + x - xoffset, y0 + y - yoffset, c );
    if ( y != 0 ){
      scanline( bmp, x0 - x, x0 + x - xoffset, y0 - y, c );
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
    scanline( bmp, x0 - x, x0 + x - xoffset, y0 + y - yoffset, c );
    if ( y != 0 ){
      scanline( bmp, x0 - x, x0 + x - xoffset, y0 - y, c );
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

void fill_polygon( Bitmap& bmp, const std::vector<IntPoint>& inPoints, const faint::Color& c ){
  if ( inPoints.empty() ){
    return;
  }
  IntRect r = bounding_rect( inPoints );
  std::vector<IntPoint> points(inPoints);
  points.push_back(points[0]);
  int minX = r.Left() - 1;
  int maxX = std::min(r.Right(), static_cast<int>(bmp.m_w) - 1);
  int minY = std::max(0, r.Top());
  int maxY = std::min( r.Bottom(), static_cast<int>(bmp.m_h) - 1);
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
      if ( y0 < y && y <= y1 || y1 < y && y <= y0 ){
	if ( x0 == x1 ){
	  x_vals.push_back(x0);
	  continue;
	}
	faint::coord k = faint::coord( y1 - y0 ) / ( x1 - x0 );
	faint::coord m = y0 - k * x0;
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
	    put_pixel_raw( bmp, x, y, c );
	  }
	  break;
	}
      }
    }
  }
}

void fill_rect( Bitmap& bmp, const IntRect& r, const faint::Color& c ){
  const int x0 = r.x;
  const int y0 = r.y;
  const int x1 = r.Right();
  const int y1 = r.Bottom();
  for ( int y = y0; y <= y1; y++ ){
    scanline( bmp, x0,
      x1, // Fixme: This was x0 + w before, but r.Right() is x0 + w -1
      y, c );
  }
}

void fill_triangle( Bitmap& bmp, const Point& p0, const Point& p1, const Point& p2, const Color& c ){
  faint::coord x0 = p0.x;
  faint::coord y0 = p0.y;
  faint::coord x1 = p1.x;
  faint::coord y1 = p1.y;
  faint::coord x2 = p2.x;
  faint::coord y2 = p2.y;

  // Fixme: Why floating point type?
  sort_y( x0, y0, x1, y1, x2, y2 );
  assert( y0 <= y1 && y1 <= y2 );
  faint::coord dx1 = ( y1 - y0 > 0 ) ? ( x1 - x0 ) / ( y1 - y0 ) : 0;
  faint::coord dx2 = ( y2 - y0 > 0 ) ? ( x2 - x0 ) / ( y2 - y0 ) : 0;
  faint::coord dx3 = ( y2 - y1 > 0 ) ? ( x2 - x1 ) / ( y2 - y1 ) : 0;

  faint::coord right = x0;
  faint::coord y = y0;
  faint::coord x = x0;

  if ( dx1 > dx2 ){
    for ( ; y < y1; y++, x += dx2, right += dx1 ){
      scanline_fl( bmp, x + 0.5, right + 0.5, y + 0.5, c );
    }
    right = x1;
    for(; y < y2; y++, x +=dx2,right += dx3){
      scanline_fl( bmp, x + 0.5, right + 0.5, y + 0.5, c );
    }
  }
  else {
    for ( ; y < y1; y++, x+= dx1, right += dx2 ){
      scanline_fl( bmp, x + 0.5, right + 0.5, y + 0.5, c );
    }
    x = x1;
    for ( ; y < y2; y++, x += dx3, right += dx2 ){
      scanline_fl( bmp, x + 0.5, right + 0.5, y + 0.5, c );
    }
  }
}

BITMAPRETURN flip_horizontal( const Bitmap& src ){
  Bitmap dst( src );
  uchar* pDst = dst.m_data;
  uchar* pSrc = src.m_data;
  for ( uint y = 0; y != src.m_h; y++ ){
    for ( uint x = 0 ; x != src.m_w; x++ ){
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

BITMAPRETURN flip_vertical( const Bitmap& src ){
  Bitmap dst( src );
  uchar* pDst = dst.m_data;
  uchar* pSrc = src.m_data;
  for ( uint y = 0; y != src.m_h; y++ ){
    for ( uint x = 0 ; x != src.m_w; x++ ){
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

BITMAPRETURN flip( const Bitmap& bmp, Axis::type axis ){
  return axis == Axis::HORIZONTAL ?
    flip_horizontal(bmp) :
    flip_vertical(bmp);
}

  void flood_fill( Bitmap& bmp, const IntPoint& pos, const faint::Color& fillColor ){
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
	if ( w.y != static_cast<int>(bmp.m_h - 1) && get_color_raw( bmp, w.x, w.y + 1 ) == targetColor ){
	  v.push_back( IntPoint( w.x, w.y + 1 ) );
	}
      }

      for ( EVER ){
	put_pixel( bmp, e, fillColor );
	if ( e.y != 0 && get_color_raw( bmp, e.x, e.y - 1 ) == targetColor ){
	  v.push_back( IntPoint( e.x, e.y - 1 ) );
	}
	if ( e.y != static_cast<int>(bmp.m_h - 1) && get_color_raw( bmp, e.x, e.y + 1 ) == targetColor ){
	  v.push_back( IntPoint( e.x, e.y + 1 ) );
	}

	if ( e.x + 1 == static_cast<int>(bmp.m_w) || get_color_raw( bmp, e.x + 1, e.y ) != targetColor ){
	  break;
	}
	e.x += 1;
      }
    }
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

const faint::Bitmap& get_null_bitmap(){
  static faint::Bitmap nullBitmap = faint::Bitmap();
  return nullBitmap;
}

std::vector<faint::Color> get_palette(const faint::Bitmap& bmp){
  std::set<faint::Color> colors;
  for ( uint y = 0; y != bmp.m_h; y++ ){
    for ( uint x = 0; x != bmp.m_w; x++ ){
      colors.insert(get_color_raw(bmp, x, y));
    }
  }
  return std::vector<faint::Color>(colors.begin(), colors.end());
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

void invert( Bitmap& bmp ){
  for ( size_t y = 0; y != bmp.m_h; y++ ){
    uchar* data = bmp.m_data + y * bmp.m_row_stride;
    for ( size_t x = 0; x != bmp.m_w * 4; x += 4 ){
      uchar* pos = data + x;
      *(pos + iR) = static_cast<uchar>(255 - *(pos + iR));
      *(pos + iG) = static_cast<uchar>(255 - *(pos + iG));
      *(pos + iB) = static_cast<uchar>(255 - *(pos + iB));
    }
  }
}

bool is_blank( const Bitmap& bmp ){
  faint::Color color = get_color( bmp, IntPoint(0,0) );
  for ( size_t y = 0; y != bmp.m_h; y++ ){
    for ( size_t x = 0; x != bmp.m_w; x++ ){
      if ( get_color( bmp, IntPoint(x,y) ) != color ){
        return false;
      }
    }
  }
  return true;
}

bool point_in_bitmap( const Bitmap& bmp, const IntPoint& pos ){
  return pos.x >= 0 && pos.y >= 0 && pos.x < static_cast<int>(bmp.m_w) && pos.y < static_cast<int>(bmp.m_h);
}

void put_pixel( Bitmap& bmp, const IntPoint& pos, const Color& color ){
  put_pixel_raw(bmp, pos.x, pos.y, color);
}

void put_pixel_raw( Bitmap& bmp, int x, int y, const Color& color ){
  if ( x < 0 || y < 0 || x >= static_cast<int>(bmp.m_w) || y >= static_cast<int>(bmp.m_h) ) {
    return;
  }
  uchar* data = bmp.GetRaw();
  int pos = y * bmp.m_row_stride + x * 4;
  data[pos + iA] = color.a;
  data[pos + iR] = color.r;
  data[pos + iG] = color.g;
  data[pos + iB] = color.b;
}

void replace_color( Bitmap& bmp, const OldColor& oldColor, const NewColor& newColor ){
  const faint::Color& c1(oldColor.Get());
  const faint::Color& c2(newColor.Get());
  for ( size_t y = 0; y != bmp.m_h; y++ ){
    uchar* data = bmp.m_data + y * bmp.m_row_stride;
    for ( size_t x = 0; x != bmp.m_w * 4; x += 4 ){
      uchar* pos = data + x;
      uchar* r = pos +iR;
      uchar* g = pos + iG;
      uchar* b = pos + iB;
      uchar* a = pos + iA;
      if ( *r == c1.r && *g == c1.g && *b == c1.b && *a == c1.a ){
	*r = c2.r;
	*g = c2.g;
	*b = c2.b;
	*a = c2.a;
      }
    }
  }
}

// Note: rotate(...) is implemented in rotate.cpp

BITMAPRETURN rotate_90cw( const Bitmap& src ){
  Bitmap dst( cairo_compatible_bitmap( transposed( src.GetSize())));
  uchar* pDst = dst.m_data;
  uchar* pSrc = src.m_data;
  for ( uint y = 0; y != src.m_h; y++ ){
    for ( uint x = 0 ; x != src.m_w; x++ ){
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

BITMAPRETURN scale(const Bitmap& bmp, const Scale& scale, ScaleQuality::type quality){
  switch ( quality ){
  case ScaleQuality::NEAREST:
    return scale_nearest(bmp, scale);
  case ScaleQuality::BILINEAR:
    return scale_bilinear(bmp, scale);
  };
  assert( false );
  return Bitmap();
}

BITMAPRETURN scale_bilinear( const Bitmap& src, const Scale& scale ){
  IntSize newSize(truncated(src.m_w * fabs(scale.x)),
    truncated(src.m_h * fabs(scale.y)));
  newSize.w = std::max(newSize.w, 1);
  newSize.h = std::max(newSize.h, 1);
  if ( newSize.w == src.m_w && newSize.h == src.m_h ){
    faint::Bitmap copy(src);
    return copy;
  }
  Bitmap scaled = cairo_compatible_bitmap(newSize);
  Colors a, b, c, d;
  int index;
  faint::coord x_ratio = floated(src.m_w) / floated(newSize.w);
  faint::coord y_ratio = floated(src.m_h) / floated(newSize.h);
  faint::coord x_diff, y_diff;
  faint::uchar blue, red, green, alpha;

  const uchar* data = src.m_data;
  for ( int i = 0; i != newSize.h; i++ ){
    for ( int j = 0; j != newSize.w; j++ ){
      int x = truncated(x_ratio * j);
      int y = truncated(y_ratio * i);
      x_diff = x_ratio * j - x;
      y_diff = y_ratio * i - y;
      index = y * src.m_row_stride + x * 4;

      a.Set( data + index );
      b.Set( data + index + 4 );
      c.Set( data + index + src.m_row_stride );
      d.Set( data + index + src.m_row_stride + 4 );

      blue = static_cast<faint::uchar>((a.b)*(1-x_diff)*(1-y_diff) + (b.b)*(x_diff)*(1-y_diff) +
	(c.b)*(y_diff)*(1-x_diff) + (d.b)*(x_diff*y_diff));

      green = static_cast<faint::uchar>( a.g*(1-x_diff)*(1-y_diff) + b.g*(x_diff)*(1-y_diff) +
	(c.g)*(y_diff)*(1-x_diff) + (d.g)*(x_diff*y_diff) );

      red = static_cast<faint::uchar>( a.r * (1-x_diff)*(1-y_diff) + b.r*(x_diff)*(1-y_diff) +
	c.r*(y_diff)*(1-x_diff) + (d.r)*(x_diff*y_diff) );

      alpha = static_cast<faint::uchar>( a.a * (1-x_diff)*(1-y_diff) + b.a*(x_diff)*(1-y_diff) +
	c.a*(y_diff)*(1-x_diff) + (d.a)*(x_diff*y_diff) );

      uchar* rDst = scaled.m_data + i * ( scaled.m_row_stride ) + j * 4;

      *( rDst + iR ) = red;
      *( rDst + iG ) = green;
      *( rDst + iB ) = blue;
      *( rDst + iA ) = alpha;
    }
  }

  if ( scale.x < 0 ){
    scaled = flip_horizontal( scaled );
  }
  if ( scale.y < 0 ) {
    scaled = flip_vertical( scaled );
  }
  return scaled;
}

BITMAPRETURN scaled_subbitmap( const Bitmap& src, const Scale& scale, const IntRect& r ){
  assert(r.x >= 0 );
  assert(r.y >= 0 );
  IntSize newSize( rounded(r.w * scale.x), rounded(r.h * scale.y));
  newSize.w = std::max(newSize.w, 1);
  newSize.h = std::max(newSize.h, 1);
  Bitmap scaled = cairo_compatible_bitmap(newSize);
  Colors a, b, c, d;
  faint::coord x_ratio = 1.0 / scale.x;
  faint::coord y_ratio = 1.0 / scale.y;

  const uchar* data = src.m_data;
  for ( int i = 0; i != newSize.h; i++ ){
    for ( int j = 0; j != newSize.w; j++ ){
      int x = truncated( x_ratio * j + r.x );
      int y = truncated( y_ratio * i + r.y );
      faint::coord x_diff = ( x_ratio * j + r.x ) - x;
      faint::coord y_diff = ( y_ratio * i + r.y ) - y;
      int index = y * src.m_row_stride + x * 4;

      a.Set( data + index );
      b.Set( data + index + 4 );
      c.Set( data + index + src.m_row_stride );
      d.Set( data + index + src.m_row_stride + 4 );

      faint::uchar blue = static_cast<faint::uchar>((a.b)*(1-x_diff)*(1-y_diff) + (b.b)*(x_diff)*(1-y_diff) +
	(c.b)*(y_diff)*(1-x_diff) + (d.b)*(x_diff*y_diff));

      faint::uchar green = static_cast<faint::uchar>( a.g*(1-x_diff)*(1-y_diff) + b.g*(x_diff)*(1-y_diff) +
	(c.g)*(y_diff)*(1-x_diff) + (d.g)*(x_diff*y_diff) );

      faint::uchar red = static_cast<faint::uchar>( a.r * (1-x_diff)*(1-y_diff) + b.r*(x_diff)*(1-y_diff) +
	c.r*(y_diff)*(1-x_diff) + (d.r)*(x_diff*y_diff) );

      faint::uchar alpha = static_cast<faint::uchar>( a.a * (1-x_diff)*(1-y_diff) + b.a*(x_diff)*(1-y_diff) +
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

BITMAPRETURN scale_nearest( const Bitmap& src, int scale ){
  const int w2 = src.m_w * scale;
  const int h2 = src.m_h * scale;

  Bitmap scaled = cairo_compatible_bitmap( IntSize(w2, h2) );
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

BITMAPRETURN scale_nearest( const Bitmap& src, const Scale& scale ){
  const int w2 = static_cast<int>( src.m_w * scale.x );
  const int h2 = static_cast<int>( src.m_h * scale.y );

  Bitmap scaled = cairo_compatible_bitmap( IntSize(w2, h2) );
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
} // namespace
