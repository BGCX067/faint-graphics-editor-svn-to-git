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

#ifndef FAINT_BITMAP_HH
#define FAINT_BITMAP_HH
#include "geo/geotypes.hh"

namespace faint{
  typedef unsigned int uint;
  extern const uint bpp;
  enum BitmapFormat{ARGB32, RGB24, INVALID_BMP_FORMAT};

  extern const int iR;
  extern const int iG;
  extern const int iB;
  extern const int iA;

  class Bitmap {
  public:
    Bitmap( BitmapFormat, uint w, uint h, uint stride );
    Bitmap( const Bitmap& );
    Bitmap();
    #ifdef FAINT_RVALUE_REFERENCES
    Bitmap( Bitmap&& );
    #endif
    ~Bitmap();
    inline uchar* GetRaw(){
      return m_data;
    }

    inline const uchar* GetRaw() const{
      return m_data;
    }

    IntSize Size() const{
      return IntSize( static_cast<int>(m_w), static_cast<int>(m_h) );
    }

    Bitmap& operator=( const Bitmap& );
    BitmapFormat m_format;
    uint m_row_stride;
    uint m_w;
    uint m_h;

    uchar* m_data;
  };

  class Brush {
  public:
    Brush( uint w, uint h, bool fill=true );
    Brush( const Brush& );
    ~Brush();
    void Set( uint x, uint y, uchar value );
    uchar Get( uint x, uint y ) const;
    uchar* m_data;
    uint m_w;
    uint m_h;
  };

  void PutPixel( Bitmap&, int x, int y, const Color& );
  void Clear( Bitmap&, const Color& );
  void DrawLine( Bitmap&, int x0, int y0, int x1, int y1, const Color& );
  void DrawLine( Bitmap&, int x0, int y0, int x1, int y1, const Color&, int lineWidth );
  void DrawFaintCircle( Bitmap&, int x0, int y0, int radius );
  void Stroke( Bitmap&, int x0, int y0, int x1, int y1, const Brush& b, const Color& );
  void DrawBitmap( Bitmap& dst, Bitmap& src, int x, int y );
  void DrawBitmap( Bitmap& dst, Bitmap& src, const Color& maskColor, int x, int y );
  void BlendBitmap( Bitmap& dst, const Bitmap& src, int x, int y );
  void BlendBitmapReal( Bitmap& dst, const Bitmap& src, int x, int y );
  void BlendBitmap( Bitmap& dst, const Bitmap& src, const Color& maskColor, int x, int y );
  Color GetColor( const Bitmap&, const IntPoint& pos );
  Color GetColor( const Bitmap&, int x, int y );
  void FloodFill( Bitmap&, int x, int y, Color fillColor );
  Brush CircleBrush( uint w );
  Bitmap ScaleNearest( const Bitmap& src, int scale );
  Bitmap ScaleNearest( const Bitmap&, faint::coord scaleX, faint::coord scaleY );  
  Bitmap ScaleBilinear( const Bitmap&, faint::coord scaleX, faint::coord scaleY );
  Bitmap FlipHorizontal( const Bitmap& );
  Bitmap FlipVertical( const Bitmap& );
  Bitmap Rotate90CW( const Bitmap& );
  void FillTriangle( Bitmap&, faint::coord x0, faint::coord y0, faint::coord x1, faint::coord y1, faint::coord x2, faint::coord y2, const Color& );
  void DrawRect( Bitmap&, int x, int y, int w, int h, const faint::Color&, int lineWidth );
  void DrawRect( Bitmap&, const Point& p0, const Point& p1, const faint::Color&, int lineWidth );

  void FillEllipse( Bitmap&, int x0, int y0, int a, int b, const faint::Color& );
  void FillEllipse( Bitmap&, const Point& p0, const Point& p1, const faint::Color& );
  void DrawEllipse( Bitmap&, int x0, int y0, int a, int b, const faint::Color& );
  void DrawEllipse( Bitmap&, int x0, int y0, int a, int b, int lineWidth, const faint::Color& );
  void DrawEllipse( Bitmap&, const Point& p0, const Point& p1, int lineWidth, const faint::Color& );
  void FillRect( Bitmap&, int x, int y, int w, int h, const faint::Color& );
  void FillRect( Bitmap&, const Point& p0, const Point& p1, const faint::Color& );
  void DesaturateSimple( Bitmap& );
  void DesaturateWeighted( Bitmap& );
  void Invert( Bitmap& );
  void ReplaceColor( Bitmap&, const Color& oldColor, const Color& newColor  );
  Bitmap AlphaBlended( const Bitmap&, const Color& );
  void BlendAlpha( Bitmap&, const Color& );
}

#endif
