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
#include "geo/intsize.hh"
#include "geo/basic.hh"
#include "util/commonfwd.hh"
#include "util/settingid.hh"

typedef Order<faint::Color>::New NewColor;
typedef Order<faint::Color>::Old OldColor;

namespace faint{
  extern const uint bpp;
  enum BitmapFormat{ARGB32, RGB24, INVALID_BMP_FORMAT};

  extern const int iR;
  extern const int iG;
  extern const int iB;
  extern const int iA;

  class Bitmap {
  public:
    Bitmap( BitmapFormat, const IntSize&, uint stride );
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

    IntSize GetSize() const{
      return IntSize( static_cast<int>(m_w), static_cast<int>(m_h) );
    }

    inline uint GetStride() const{
      return m_row_stride;
    }



    Bitmap& operator=( const Bitmap& );
    #ifdef FAINT_RVALUE_REFERENCES
    Bitmap& operator=( Bitmap&& );
    #endif

    BitmapFormat m_format;
    uint m_row_stride;
    uint m_w;
    uint m_h;

    uchar* m_data;
  };
} // namespace faint

// Macro for allowing toggling whether bitmaps are returned by
// C+11-rvalue references or by copy from functions.
#ifdef FAINT_RVALUE_REF
#define BITMAPRETURN faint::Bitmap&&
#else
#define BITMAPRETURN faint::Bitmap
#endif

class DstBmp{
public:
  explicit DstBmp(faint::Bitmap& bmp) :
    m_bmp(bmp)
  {}
  faint::uchar* GetRaw(){
    return m_bmp.m_data;
  }
  IntSize GetSize() const{
    return m_bmp.GetSize();
  }
  faint::uint GetStride() const{
    return m_bmp.m_row_stride;
  }
private:
  DstBmp& operator=(const DstBmp&);
  faint::Bitmap& m_bmp;
};

inline DstBmp onto(faint::Bitmap& bmp){
  return DstBmp(bmp);
}


namespace faint{
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

  BITMAPRETURN alpha_blended( const Bitmap&, const Color& );
  bool bitmap_ok( const faint::Bitmap& bmp );
  void blend_alpha( Bitmap&, const Color& );
  void blend( const Bitmap& src, DstBmp, const IntPoint& topLeft );
  void blend_masked( const Bitmap& src, DstBmp, const Color& maskColor, const IntPoint& topLeft );
  Brush circle_brush( uint w );
  void clear( Bitmap&, const Color& );
  size_t count_colors( const Bitmap& bmp );
  void desaturate_simple( Bitmap& );
  void desaturate_weighted( Bitmap& );
  void blit( const Bitmap&, DstBmp, const IntPoint& topLeft );
  void blit_masked( const Bitmap& src, DstBmp, const Color&, const IntPoint& topLeft );
  void draw_ellipse( Bitmap&, const IntRect&, int lineWidth, const Color& );
  void draw_faint_circle( Bitmap&, const IntPoint& center, int radius );
  void draw_line( Bitmap&, const IntPoint&, const IntPoint&, const Color&, int lineWidth, bool dashes, LineCap::type );
  void draw_polygon( Bitmap&, const std::vector<IntPoint>&, const Color&, int lineWidth, bool dashes );
  void draw_polyline( Bitmap&, const std::vector<IntPoint>&, const Color&, int lineWidth, bool dashes, LineCap::type );
  void draw_rect( Bitmap&, const IntRect&, const Color&, int lineWidth, bool dashes );
  void erase_but( Bitmap&, const faint::Color& keep, const faint::Color& eraserColor );
  bool inside( const IntRect&, const Bitmap& );
  void fill_ellipse( Bitmap&, const IntRect&, const Color& );
  void fill_polygon( Bitmap&, const std::vector<IntPoint>&, const faint::Color& );
  void fill_rect( Bitmap&, const IntRect&, const Color& );
  void fill_triangle( Bitmap&, const Point&, const Point&, const Point&, const Color& );
  // Fixme: I guess Axis may suggest this flips over the axis, it does not.
  BITMAPRETURN flip( const Bitmap&, Axis::type );
  void flood_fill( Bitmap&, const IntPoint&, const Color& fillColor );
  Color get_color( const Bitmap&, const IntPoint& pos );
  Color get_color_raw( const Bitmap&, int x, int y );
  const Bitmap& get_null_bitmap();
  std::vector<Color> get_palette( const Bitmap& );
  void invert( Bitmap& );
  bool is_blank( const Bitmap& );
  bool point_in_bitmap( const Bitmap&, const IntPoint& );
  void put_pixel( Bitmap&, const IntPoint&, const Color& );
  void put_pixel_raw( Bitmap&, int x, int y, const Color& );
  void replace_color( Bitmap&, const OldColor&, const NewColor& );
  BITMAPRETURN rotate( const Bitmap&, radian angle, const Color& background );
  BITMAPRETURN rotate_90cw( const Bitmap& );
  BITMAPRETURN scale( const Bitmap&, const Scale&, ScaleQuality::type );
  BITMAPRETURN scaled_subbitmap( const Bitmap&, const Scale&, const IntRect& );
  BITMAPRETURN scale_bilinear( const Bitmap&, const Scale& );
  BITMAPRETURN scale_nearest( const Bitmap&, int scale );
  BITMAPRETURN scale_nearest( const Bitmap&, const Scale& );
  void stroke( Bitmap&, const IntPoint& p0, const IntPoint& p1, const Brush&, const Color& );
}

#endif
