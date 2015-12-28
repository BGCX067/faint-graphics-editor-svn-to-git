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

#include "bitmap.h"
#include "cairo_util.h"
#include "geo/geotypes.hh"
#include "util/angle.hh"
#include <vector>
#include <cassert>
#include <cstring>
#include <map>

using std::swap;
using std::abs;

namespace faint{
  const uint bpp = 4;
  const int iR = 2;
  const int iG = 1;
  const int iB = 0;
  const int iA = 3;

  int int_abs( int v ){
    return v < 0 ? -v : v;
  }

  Bitmap::Bitmap( BitmapFormat format, uint w, uint h, uint stride ):
    m_format( format ),
    m_row_stride( stride ),
    m_w( w ),
    m_h( h ),
    m_data(0)
  {
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
    m_data = new uchar[ m_row_stride * m_h ];
    memcpy( m_data, other.m_data, m_row_stride * m_h );
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
    m_format = other.m_format;
    m_w = other.m_w;
    m_h = other.m_h;
    m_row_stride = other.m_row_stride;
    m_data = new uchar[ m_row_stride * m_h ];
    memcpy( m_data, other.m_data, m_row_stride * m_h );
    return *this;
  }

  void PutPixel( Bitmap& bmp, int x, int y, const Color& color ){
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

  void BlendPixel( Bitmap& bmp, int x, int y, const Color& color ){
    if ( color.a == 255 ){
      PutPixel( bmp, x, y, color );
    }
    if ( x < 0 || y < 0 || x >= static_cast<int>(bmp.m_w) || y >= static_cast<int>(bmp.m_h) ) {
      return;
    }
    uchar* data = bmp.GetRaw();
    int pos = y * bmp.m_row_stride + x * 4;

    uchar r = data[pos + iR];
    uchar g = data[pos + iG];
    uchar b = data[pos + iB];

    data[pos + iR] = static_cast<uchar>(( color. r * color.a + r * ( 255 - color.a ) ) / 255);
    data[pos + iG] = static_cast<uchar>(( color. g * color.a + g * ( 255 - color.a ) ) / 255);
    data[pos + iB] = static_cast<uchar>(( color. b * color.a + b * ( 255 - color.a ) ) / 255);
    data[pos + iA] = 255;
  }

  inline void BrushStroke( Bitmap& bmp, int x, int y, const Brush& b, const Color& c ){
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
          PutPixel( bmp, x + xB - xCenter, y + yB - yCenter, c );
        }
      }
    }
  }

  void DrawLine( Bitmap& bmp, int x0, int y0, int x1, int y1, const Color& c ){
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
    for ( int x = x0; x<= x1; x++ ){
      err += dy;
      if ( steep ){
        PutPixel( bmp, y, x, c );
      }
      else {
        PutPixel( bmp, x, y, c );
      }
      if ( 2 * err > dx ){
        y += yStep;
        err -= dx;
      }
    }
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

  void scanline( Bitmap& bmp, int x0, int x1, int y, const faint::Color& c ){
    if ( x0 > x1 ){
      swap(x0,x1);
    }
    for ( int x = x0; x <= x1; x++ ){
      PutPixel( bmp, x, y, c );
    }
  }

  void scanline_fl( Bitmap& bmp, faint::coord x0, faint::coord x1, faint::coord y, const faint::Color& c ){
    scanline( bmp, static_cast<int>(x0), static_cast<int>(x1), static_cast<int>(y), c );
  }

  void vertical( Bitmap& bmp, int y0, int y1, int x, const faint::Color& c ){
    if ( y0 > y1 ){
      swap(y0,y1);
    }
    for ( int y = y0; y <= y1; y++ ){
      PutPixel( bmp, x, y, c );
    }
  }

  void FillTriangle( Bitmap& bmp, faint::coord x0, faint::coord y0, faint::coord x1, faint::coord y1, faint::coord x2, faint::coord y2, const Color& c ){
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

  void DrawRect( Bitmap& bmp, int x, int y, int w, int h, const faint::Color& c, int lineWidth ){
    lineWidth = std::max(1, lineWidth);
    for ( int i = 0; i != lineWidth; i++ ){
      scanline( bmp, x, x+w - 1, y + i, c );
      scanline( bmp, x, x+w - 1, y + h - i - 1, c );
      vertical( bmp, y, y + h - 1, x + i, c );
      vertical( bmp, y, y + h - 1, x + w - i - 1, c );
    }
  }

  void DrawRect( Bitmap& bmp, const Point& p0, const Point& p1, const faint::Color& c, int lineWidth ){
    lineWidth = std::max(1, lineWidth);
    int x0 = static_cast<int>(std::min( p0.x, p1.x ));
    int x1 = static_cast<int>(std::max( p0.x, p1.x ));
    int y0 = static_cast<int>(std::min( p0.y, p1.y ));
    int y1 = static_cast<int>(std::max( p0.y, p1.y ));
    for ( int i = 0; i != lineWidth; i++ ){
      scanline( bmp, x0, x1, y0 + i, c );
      scanline( bmp, x0, x1, y1 - i, c );
      vertical( bmp, y0, y1, x0 + i, c );
      vertical( bmp, y0, y1, x1 - i, c );
    }
  }

  void FillRect( Bitmap& bmp, int x0, int y0, int w, int h, const faint::Color& c ){
    for ( int y = y0; y != y0 + h; y++ ){
      scanline( bmp, x0, x0 + w, y, c );
    }
  }

  void FillRect( Bitmap& bmp, const Point& p0, const Point& p1, const faint::Color& c ){
    int x0 = static_cast<int>(std::min( p0.x, p1.x ));
    int x1 = static_cast<int>(std::max( p0.x, p1.x ));
    int y0 = static_cast<int>(std::min( p0.y, p1.y ));
    int y1 = static_cast<int>(std::max( p0.y, p1.y ));
    for ( int y = y0; y <= y1; y++ ){
      scanline( bmp, x0, x1, y, c );
    }
  }

  void DrawLine( Bitmap& bmp, int x0, int y0, int x1, int y1, const Color& c, int lineWidth ){
    lineWidth = std::max(1, lineWidth);
    if ( lineWidth == 1 ){
      DrawLine( bmp, x0, y0, x1, y1, c );
      return;
    }

    faint::radian rads = atan2( float(x1 - x0), float(y1 - y0) );
    faint::radian adj = rads + faint::pi / 2;

    faint::coord xOff = sin(adj) * static_cast<faint::coord>(lineWidth);
    faint::coord yOff = cos(adj) * static_cast<faint::coord>(lineWidth);

    faint::Color r(255,0,0);
    faint::Color b(0,0,255);

    FillTriangle( bmp, x0 + xOff - xOff / 2, y0 + yOff - yOff / 2, x0 - xOff / 2, y0 - yOff / 2, x1 + xOff - xOff / 2, y1 + yOff - yOff / 2, c );
    FillTriangle( bmp, x0 - xOff / 2, y0 - yOff / 2, x1 + xOff - xOff / 2, y1 + yOff - yOff / 2, x1 - xOff / 2, y1 - yOff / 2, c );
  }

  void Stroke( Bitmap& bmp, int x0, int y0, int x1, int y1, const Brush& b, const Color& c ){
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
    float err = 0;
    int y = y0;
    int yStep = y0 < y1 ? 1 : -1;
    for ( int x = x0; x<= x1; x++ ){
      err += dy;
      if ( steep ){
        BrushStroke( bmp, y, x, b, c );
      }
      else {
        BrushStroke( bmp, x, y, b, c );
      }

      if ( 2 * err > dx ){
        y += yStep;
        err -= dx;
      }
    }
  }

  inline bool inside( const Bitmap& dst, const Bitmap& src, int x0, int y0 ){
    if ( x0 >= static_cast<int>(dst.m_w) || y0 >= static_cast<int>(dst.m_h) ){
      return false;
    }
    if ( x0 + static_cast<int>(src.m_w) < 0 || y0 + static_cast<int>(src.m_h) < 0 ){
      return false;
    }
    return true;

  }

  void DrawBitmap( Bitmap& dst, Bitmap& src, int x0, int y0 ){
    if ( !inside( dst, src, x0, y0 ) ){
      return;
    }

    const uint xMin = std::max( 0, -x0 );
    const uint yMin = std::max( 0, -y0 );
    const uint xMax = std::min( (int) dst.m_w - x0, (int)src.m_w );
    const uint yMax = std::min( (int)dst.m_h - y0, (int)src.m_h  );

    uint dstStride = dst.m_row_stride;
    uint srcStride = src.m_row_stride;
    uchar* dstData = dst.GetRaw();
    uchar* srcData = src.GetRaw();

    for ( uint y = yMin; y != yMax; y++ ){
      for ( uint x = xMin; x != xMax; x++ ){
        int srcPos = y * srcStride + x * 4;
        // if ( srcData[srcPos + faint::iA] == 0 ){
        //        continue;
        //      }
        int dstPos = ( y + y0 ) * dstStride + ( x + x0 ) * 4;
        dstData[ dstPos ] = srcData[srcPos ];
        dstData[ dstPos + 1 ] = srcData[srcPos + 1 ];
        dstData[ dstPos + 2 ] = srcData[srcPos + 2 ];
        dstData[ dstPos + 3 ] = srcData[srcPos + 3 ];
      }
    }
  }

  void BlendBitmap( Bitmap& dst, const Bitmap& src, int x0, int y0 ){
    if ( !inside( dst, src, x0, y0 ) ){
      return;
    }

    const uint xMin = std::max( 0, -x0 );
    const uint yMin = std::max( 0, -y0 );
    const uint xMax = std::min( (int) dst.m_w - x0, (int)src.m_w );
    const uint yMax = std::min( (int)dst.m_h - y0, (int)src.m_h  );

    uint dstStride = dst.m_row_stride;
    uint srcStride = src.m_row_stride;
    uchar* dstData = dst.GetRaw();
    const uchar* srcData = src.GetRaw();

    for ( uint y = yMin; y != yMax; y++ ){
      for ( uint x = xMin; x != xMax; x++ ){
        int srcPos = y * srcStride + x * 4;
        if ( srcData[srcPos + faint::iA] == 0 ){
          continue;
        }
        int dstPos = ( y + y0 ) * dstStride + ( x + x0 ) * 4;
        dstData[ dstPos ] = srcData[srcPos ];
        dstData[ dstPos + 1 ] = srcData[srcPos + 1 ];
        dstData[ dstPos + 2 ] = srcData[srcPos + 2 ];
        dstData[ dstPos + 3 ] = srcData[srcPos + 3 ];
      }
    }
  }

  void BlendBitmap( Bitmap& dst, const Bitmap& src, const faint::Color& maskColor, int x0, int y0 ){
    if ( !inside( dst, src, x0, y0 ) ){
      return;
    }

    const uint xMin = std::max( 0, -x0 );
    const uint yMin = std::max( 0, -y0 );
    const uint xMax = std::min( (int) dst.m_w - x0, (int)src.m_w );
    const uint yMax = std::min( (int)dst.m_h - y0, (int)src.m_h  );

    uint dstStride = dst.m_row_stride;
    uint srcStride = src.m_row_stride;
    uchar* dstData = dst.GetRaw();
    const uchar* srcData = src.GetRaw();

    for ( uint y = yMin; y != yMax; y++ ){
      for ( uint x = xMin; x != xMax; x++ ){
        int srcPos = y * srcStride + x * 4;
        if ( srcData[srcPos + faint::iA] == 0 ||
           ( srcData[ srcPos + 3 ] == maskColor.a &&
             srcData[ srcPos + 2 ] == maskColor.r &&
             srcData[ srcPos + 1 ] == maskColor.g &&
             srcData[ srcPos ] == maskColor.b ) ) {
          continue;
        }

        int dstPos = ( y + y0 ) * dstStride + ( x + x0 ) * 4;

        dstData[ dstPos ] = srcData[srcPos ];
        dstData[ dstPos + 1 ] = srcData[srcPos + 1 ];
        dstData[ dstPos + 2 ] = srcData[srcPos + 2 ];
        dstData[ dstPos + 3 ] = srcData[srcPos + 3 ];
      }
    }
  }

  void BlendBitmapReal( Bitmap& dst, const Bitmap& src, int x0, int y0 ){
    if ( !inside( dst, src, x0, y0 ) ){
      return;
    }

    const uint xMin = std::max( 0, -x0 );
    const uint yMin = std::max( 0, -y0 );
    const uint xMax = std::min( (int) dst.m_w - x0, (int)src.m_w );
    const uint yMax = std::min( (int)dst.m_h - y0, (int)src.m_h  );

    uint srcStride = src.m_row_stride;
    const uchar* srcData = src.GetRaw();

    for ( uint y = yMin; y != yMax; y++ ){
      for ( uint x = xMin; x != xMax; x++ ){
        int srcPos = y * srcStride + x * 4;
        if ( srcData[srcPos + faint::iA] == 0 ){
          continue;
        }

        // Fixme: Horrendously inefficient
        faint::Color c( srcData[srcPos + faint::iR], srcData[srcPos + faint::iG], srcData[srcPos + faint::iB],
          srcData[srcPos + faint::iA] );
        BlendPixel( dst, x, y, c );
      }
    }
  }

  void DrawBitmap( Bitmap& dst, Bitmap& src, const faint::Color& maskColor, int x0, int y0 ){
    if ( !inside( dst, src, x0, y0 ) ){
      return;
    }
    const unsigned int xMin = std::max( 0, -x0 );
    const unsigned int yMin = std::max( 0, -y0 );

    const unsigned int xMax = std::min( (int) dst.m_w - x0, (int)src.m_w );
    const unsigned int yMax = std::min( (int)dst.m_h - y0, (int)src.m_h  );
    const uint dstStride = dst.m_row_stride;
    const uint srcStride = src.m_row_stride;

    uchar* dstData = dst.GetRaw();
    uchar* srcData = src.GetRaw();

    for ( uint y = yMin; y != yMax; y++ ){
      for ( uint x = xMin; x != xMax; x++ ){
        int srcPos = y * srcStride + x * 4;
        // if ( srcData[srcPos + faint::iA] == 0 ){
        //        continue;
        //      }
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

  Bitmap ScaleNearest( const Bitmap& src, int scale ){
    const int w2 = src.m_w * scale;
    const int h2 = src.m_h * scale;

    Bitmap scaled = CairoCompatibleBitmap( w2, h2 );
    // EDIT: added +1 to account for an early rounding problem
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

  Bitmap ScaleNearest( const Bitmap& src, faint::coord scaleX, faint::coord scaleY ){
    const int w2 = static_cast<int>( src.m_w * scaleX );
    const int h2 = static_cast<int>( src.m_h * scaleY );

    Bitmap scaled = CairoCompatibleBitmap( w2, h2 );
    // EDIT: added +1 to account for an early rounding problem
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

  Bitmap ScaleBilinear( const Bitmap& src, faint::coord scaleX, faint::coord scaleY ){
    faint::coord w2 = floated(truncated(src.m_w * fabs(scaleX)));
    faint::coord h2 = floated(truncated(src.m_h * fabs(scaleY)));
    Bitmap scaled = CairoCompatibleBitmap( w2, h2 );
    Colors a, b, c, d;    
    int index;
    faint::coord x_ratio = static_cast<faint::coord>(src.m_w - 1 ) / w2;
    faint::coord y_ratio = static_cast<faint::coord>(src.m_h - 1 ) / h2;
    faint::coord x_diff, y_diff;
    faint::uchar blue, red, green, alpha;

    const uchar* data = src.m_data;
    for ( int i = 0; i != h2; i++ ){
      for ( int j = 0; j != w2; j++ ){
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
          (c.b)*(y_diff)*(1-x_diff)   + (d.b)*(x_diff*y_diff));

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

    if ( scaleX < 0 ){
      scaled = FlipHorizontal( scaled );
    }
    if ( scaleY < 0 ) {
      scaled = FlipVertical( scaled );
    }
    return scaled;
  }

  void DrawFaintCircle( Bitmap& bmp, int x0, int y0, int radius){
    int f = 1 - radius;
    int ddF_x = 1;
    int ddF_y = -2 * radius;
    int x = 0;
    int y = radius;
    faint::Color c(0,0,0);
    PutPixel( bmp, x0, y0 + radius, c);
    PutPixel( bmp, x0, y0 - radius, c);
    PutPixel( bmp, x0 + radius, y0, c);
    PutPixel( bmp, x0 - radius, y0, c);

    while( x < y ) {
      if( f >= 0 ) {
        y--;
        ddF_y += 2;
        f += ddF_y;
      }
      x++;
      ddF_x += 2;
      f += ddF_x;
      PutPixel( bmp, x0 + x, y0 + y, c );
      PutPixel( bmp, x0 - x, y0 + y, c );
      PutPixel( bmp, x0 + x, y0 - y, c );
      PutPixel( bmp, x0 - x, y0 - y, c );
      PutPixel( bmp, x0 + y, y0 + x, c );
      PutPixel( bmp, x0 - y, y0 + x, c );
      PutPixel( bmp, x0 + y, y0 - x, c );
      PutPixel( bmp, x0 - y, y0 - x, c );
    }
  }

  Brush CircleBrush( uint w ){
    if ( w <= 1 ){
      return Brush( 1, 1, true );
    }
    uint radius = w / 2;

    int f = 1 - radius;
    int ddF_x = 1;
    int ddF_y = -2 * radius;
    uint x = 0;
    uint y = radius;

    Brush b( w +1, w + 1, false );

    int cx = radius;
    int cy = radius;
    b.Set( cx, cy + radius, 255 );
    b.Set( cx, cy - radius, 255 );
    b.Set( cx + radius, cy, 255 );
    b.Set( cx - radius, cy, 255);

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
        b.Set( cx + i, cy + y, 255 );
        b.Set( cx - i, cy + y, 255 );
        b.Set( cx + i, cy - y, 255 );
        b.Set( cx - i, cy - y, 255 );
      }

      for ( size_t i = 0; i <= y; i++ ){
        b.Set( cx + i, cy + x, 255 );
        b.Set( cx - i, cy + x, 255 );
        b.Set( cx + i, cy - x, 255 );
        b.Set( cx - i, cy - x, 255 );
      }
    }
    for ( size_t i = 0; i != radius; i++ ){
      b.Set( cx - i, cy, 255 );
      b.Set( cx + i, cy, 255 );
    }
    return b;
  }

  Color GetColor( const Bitmap& bmp, int x, int y ){
    int pos = y * bmp.m_row_stride + x * 4;
    uchar* data = bmp.m_data;
    return Color( data[ pos + iR ], data[pos + iG ], data[ pos + iB ], data[ pos + iA ] );
  }

  Color GetColor( const Bitmap& bmp, const IntPoint& pos ){
    return GetColor( bmp, pos.x, pos.y );
  }

  void FloodFill( Bitmap& bmp, int x, int y, Color fillColor ){
    std::vector< IntPoint > v;
    const Color targetColor = GetColor( bmp, x, y );
    //     if ( fillColor.a != 255 ){
    //       fillColor.r = (fillColor.r * fillColor.a + targetColor.r * ( 255 - fillColor.a )) / 255;
    //       fillColor.g = (fillColor.g * fillColor.a + targetColor.g * ( 255 - fillColor.a )) / 255;
    //       fillColor.b = (fillColor.b * fillColor.a + targetColor.b * ( 255 - fillColor.a )) / 255;
    //       fillColor.a = 255;
    //     }

    if ( targetColor == fillColor ){
      return;
    }


    v.push_back( IntPoint( x, y ) );
    for ( uint i = 0; i != v.size(); i++ ){
      if ( GetColor( bmp, v[i].x, v[i].y ) == targetColor ){
        IntPoint w = v[i];
        IntPoint e = v[i];
        for ( ;; ){
          PutPixel( bmp, w.x, w.y, fillColor );
          if ( w.x - 1 < 0 || GetColor( bmp, w.x - 1, w.y ) != targetColor ){
            break;
          }
          w.x -= 1;
          if ( w.y != 0 && GetColor( bmp, w.x, w.y - 1 ) == targetColor ){
            v.push_back( IntPoint( w.x, w.y - 1 ) );
          }
          if ( w.y != static_cast<int>(bmp.m_h - 1) && GetColor( bmp, w.x, w.y + 1 ) == targetColor ){
            v.push_back( IntPoint( w.x, w.y + 1 ) );
          }
        }

        for ( ;; ){
          PutPixel( bmp, e.x, e.y, fillColor );
          if ( e.y != 0 && GetColor( bmp, e.x, e.y - 1 ) == targetColor ){
            v.push_back( IntPoint( e.x, e.y - 1 ) );
          }
          if ( e.y != static_cast<int>(bmp.m_h - 1) && GetColor( bmp, e.x, e.y + 1 ) == targetColor ){
            v.push_back( IntPoint( e.x, e.y + 1 ) );
          }

          if ( e.x + 1 == static_cast<int>(bmp.m_w) || GetColor( bmp, e.x + 1, e.y ) != targetColor ){
            break;
          }
          e.x += 1;
        }
      }
    }
  }

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

  void Clear( Bitmap& bmp, const Color& c ){
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

  void DesaturateSimple( Bitmap& bmp ){
    uchar* data = bmp.m_data;
    for ( uint y = 0; y != bmp.m_h; y++ ){
      for ( uint x = 0 ; x != bmp.m_w; x++ ){
        int dst = y * bmp.m_row_stride + x * 4;
        uchar gray = ( data[dst + iR ] + data[dst + iG ] + data[dst + iB ] ) / 3;
        data[dst + iR ] = gray;
        data[dst + iG ] = gray;
        data[dst + iB ] = gray;
      }
    }
  }

  void DesaturateWeighted( Bitmap& bmp ){
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

  Bitmap FlipHorizontal( const Bitmap& src ){
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

  Bitmap FlipVertical( const Bitmap& src ){
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

  Bitmap Rotate90CW( const Bitmap& src ){
    Bitmap dst( CairoCompatibleBitmap( src.m_h, src.m_w ) );
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

  void DrawEllipse( faint::Bitmap& bmp, const Point& p0, const Point& p1, int lineWidth, const faint::Color& c ){
    faint::coord w = abs(p0.x - p1.x) + 1;
    faint::coord h = abs(p0.y - p1.y) + 1;
    DrawEllipse( bmp,
      static_cast<int>( p0.x + w / 2 + 0.5f),
      static_cast<int>( p0.y + h / 2 + 0.5f),
      static_cast<int>( w / 2 + 0.5f ), 
      static_cast<int>( h / 2 + 0.5f), 
      lineWidth, c );
  }

  void FillEllipse( faint::Bitmap& bmp, const Point& p0, const Point& p1, const faint::Color& c ){
    faint::coord w = abs(p0.x - p1.x) + 1;
    faint::coord h = abs(p0.y - p1.y) + 1;
    FillEllipse( bmp, 
      static_cast<int>(p0.x + w / 2 + 0.5), 
      static_cast<int>(p0.y + h / 2 + 0.5), 
      static_cast<int>(w / 2 + 0.5),
      static_cast<int>(h / 2 + 0.5), 
      c );
  }

  void FillEllipse( faint::Bitmap& bmp, int x0, int y0, int a, int b, const faint::Color& c ){
    if ( a == 0 || b == 0 ){
      return;
    }
    int a2 = 2 * a * a;
    int b2 = 2 * b * b;
    int error = a * a * b;

    int x = 0;
    int y = b;

    int stopy = 0;
    int stopx = a2 * b;

    while (stopy <= stopx) {
      scanline( bmp, x0 - x, x0 + x, y0 + y, c );
      if ( y != 0 ){
        scanline( bmp, x0 - x, x0 + x, y0 - y, c );
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
      scanline( bmp, x0 - x, x0 + x, y0 + y, c );
      if ( y != 0 ){
        scanline( bmp, x0 - x, x0 + x, y0 - y, c );
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

  void DrawEllipse( faint::Bitmap& bmp, int x0, int y0, int a, int b, const faint::Color& c ){
    if ( a == 0 || b == 0 ){
      return;
    }
    int a2 = 2 * a * a;
    int b2 = 2 * b * b;
    int error = a * a * b;

    int x = 0;
    int y = b;

    int stopy = 0;
    int stopx = a2 * b;

    while (stopy <= stopx) {
      PutPixel( bmp, x0 + x, y0 + y, c );
      PutPixel( bmp, x0 - x, y0 + y, c );
      PutPixel( bmp, x0 - x, y0 - y, c );
      PutPixel( bmp, x0 + x, y0 - y, c );
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
      PutPixel( bmp, x0 + x, y0 + y, c);
      PutPixel( bmp, x0 - x, y0 + y, c);
      PutPixel( bmp, x0 - x, y0 - y, c);
      PutPixel( bmp, x0 + x, y0 - y, c);
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

  std::map<int,int> EllipsePoints( int x0, int y0, int a, int b ){
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
      //PutPixel( bmp, x0 + x, y0 + y, c );
      points[ y0 + y ] = x0 - x;
      points[ y0 - y ] = x0 - x;
      //PutPixel( bmp, x0 + x, y0 - y, c );
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

      //PutPixel( bmp, x0 + x, y0 + y, c);
      points[ y0 + y ] = x0 - x;
      points[ y0 - y ] = x0 - x;
      // PutPixel( bmp, x0 + x, y0 - y, c);
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

  void DrawEllipse( faint::Bitmap& bmp, int x0, int y0, int a, int b, int lineWidth, const faint::Color& c ){
    if ( a == 0 || b == 0 ){
      return;
    }
    if ( lineWidth == 1 ){
      DrawEllipse( bmp, x0, y0, a, b, c );
      return;
    }
    std::map<int,int> points = EllipsePoints( x0, y0, a - lineWidth, b - lineWidth );
    for ( std::map<int,int>::iterator it = points.begin(); it != points.end(); ++it ){
      PutPixel( bmp, it->second, it->first, c );
    }

    int a2 = 2 * a * a;
    int b2 = 2 * b * b;
    int error = a * a * b;

    int x = 0;
    int y = b;

    int stopy = 0;
    int stopx = a2 * b;

    while (stopy <= stopx) {
      PutPixel( bmp, x0 + x, y0 + y, c );

      if ( points.find( y0 + y ) != points.end() ){
        int x1 = points[y0 + y];
        scanline( bmp, x0 - x, x1, y0 + y, c );
        int dx = x1 - ( x0 - x );
        scanline( bmp, x0 + x - dx, x0 + x, y0 + y, c );

      }
      else {
        scanline( bmp, x0 - x, x0 + x, y0 + y, c );
      }


      if ( points.find( y0 - y ) != points.end() ){
        int x1 = points[y0 - y];
        scanline( bmp, x0 - x, x1, y0 - y, c );
        int dx = x1 - ( x0 - x );
        scanline( bmp, x0 + x - dx, x0 + x, y0 - y, c);

      }
      else {
        scanline( bmp, x0 - x, x0 + x, y0 - y, c );
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
        scanline( bmp, x0 - x, x1, y0 + y, c );
        int dx = x1 - ( x0 - x );
        scanline( bmp, x0 + x - dx, x0 + x, y0 + y, c );
      }
      else {
        scanline( bmp, x0 - x, x0 + x, y0 + y, c );
      }

      if ( points.find( y0 - y ) != points.end() ){
        int x1 = points[y0 - y];
        scanline( bmp, x0 - x, x1, y0 - y, c );
        int dx = x1 - ( x0 - x );
        scanline( bmp, x0 + x - dx, x0 + x, y0 - y, c );
      }
      else {
        scanline( bmp, x0 - x, x0 + x, y0 - y, c );
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

  void Invert( Bitmap& bmp ){
    for ( size_t y = 0; y != bmp.m_h; y++ ){
      uchar* data = bmp.m_data + y * bmp.m_row_stride;
      for ( size_t x = 0; x != bmp.m_w * 4; x += 4 ){
        uchar* pos = data + x;
        *(pos + iR) = 255 - *(pos + iR);
        *(pos + iG) = 255 - *(pos + iG);
        *(pos + iB) = 255 - *(pos + iB);
      }
    }
  }

  void ReplaceColor( Bitmap& bmp, const Color& c1, const Color& c2 ){
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

  Bitmap AlphaBlended( const Bitmap& src, const Color& color ){
    faint::Bitmap dst( CairoCompatibleBitmap( src.m_w, src.m_h ) );
    Clear( dst, color );
    BlendBitmapReal( dst, src, 0, 0 );
    return dst;
  }

  void BlendAlpha( Bitmap& bmp, const Color& color ){
    bmp = AlphaBlended( bmp, color );
  }
}
