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
#include "geo/basic.hh"
#include "geo/intsize.hh"
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
  Bitmap();
  Bitmap( const Bitmap& );
  Bitmap( const IntSize& );
  Bitmap( const IntSize&, const Color& );
  Bitmap( BitmapFormat, const IntSize&, uint stride );
  Bitmap( Bitmap&& );
  ~Bitmap();
  inline uchar* GetRaw(){
    return m_data;
  }

  inline const uchar* GetRaw() const{
    return m_data;
  }

  IntSize GetSize() const{
    return IntSize(m_w, m_h);
  }

  inline uint GetStride() const{
    return m_row_stride;
  }

  Bitmap& operator=( const Bitmap& );
  Bitmap& operator=( Bitmap&& );

  BitmapFormat m_format;
  uint m_row_stride;
  int m_w;
  int m_h;
  uchar* m_data;
};

} // namespace faint

class DstBmp{
  // For indicating a target bitmap in Bitmap operations,
  // Should be created using the onto(faint::Bitmap&)-function
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
  Brush( int w, int h, bool fill=true );
  Brush( const Brush& );
  ~Brush();
  void Set( int x, int y, uchar value );
  uchar Get( int x, int y ) const;
  uchar* m_data;
  int m_w;
  int m_h;
};

Bitmap alpha_blended( const Bitmap&, const Color& );
Bitmap alpha_blended( const Bitmap&, const ColRGB& );
bool bitmap_ok( const Bitmap& bmp );
void blend_alpha( Bitmap&, const Color& );
void blend( const Bitmap& src, DstBmp, const IntPoint& topLeft );
void blend_masked( const Bitmap& src, DstBmp, const Color& maskColor, const IntPoint& topLeft );
void blit( const Bitmap&, DstBmp, const IntPoint& topLeft );
void blit_masked( const Bitmap& src, DstBmp, const Color&, const IntPoint& topLeft );
Brush circle_brush( uint w );
void clear( Bitmap&, const Color& );
void clear( Bitmap&, const ColRGB& );
void clear( Bitmap&, const DrawSource& );
size_t count_colors( const Bitmap& bmp );
void draw_ellipse( Bitmap&, const IntRect&, int lineWidth, const DrawSource&, bool dashes );
void draw_line( Bitmap&, const IntPoint&, const IntPoint&, const DrawSource&, int lineWidth, bool dashes, LineCap );
void draw_line_color( Bitmap&, const IntPoint&, const IntPoint&, const Color&, int lineWidth, bool dashes, LineCap );
void draw_polygon( Bitmap&, const std::vector<IntPoint>&, const DrawSource&, int lineWidth, bool dashes );
void draw_polyline( Bitmap&, const std::vector<IntPoint>&, const DrawSource&, int lineWidth, bool dashes, LineCap );
void draw_rect( Bitmap&, const IntRect&, const DrawSource&, int lineWidth, bool dashes );
void draw_rect_color( Bitmap&, const IntRect&, const Color&, int lineWidth, bool dashes );
void erase_but( Bitmap&, const Color& keep, const DrawSource& eraser );
void fill_ellipse( Bitmap&, const IntRect&, const DrawSource& );
void fill_ellipse_color( Bitmap&, const IntRect&, const Color& );
void fill_polygon( Bitmap&, const std::vector<IntPoint>&, const DrawSource& );
void fill_rect( Bitmap&, const IntRect&, const DrawSource& );
void fill_rect_color( Bitmap&, const IntRect&, const Color& );
void fill_rect_rgb( Bitmap&, const IntRect&, const ColRGB& );
void fill_triangle( Bitmap&, const Point&, const Point&, const Point&, const DrawSource& );
void fill_triangle_color( Bitmap&, const Point&, const Point&, const Point&, const Color& );

// Fixme: I guess Axis may suggest this flips over the axis, it does not.
Bitmap flip( const Bitmap&, Axis );
void flood_fill( Bitmap&, const IntPoint&, const Color& fillColor );
void flood_fill2( Bitmap&, const IntPoint&, const DrawSource& ); // Fixme?
Color get_color( const Bitmap&, const IntPoint& );
Color get_color_raw( const Bitmap&, int x, int y );
Bitmap get_null_bitmap();
std::vector<Color> get_palette( const Bitmap& );
bool inside( const IntRect&, const Bitmap& );
bool is_blank( const Bitmap& );
bool point_in_bitmap( const Bitmap&, const IntPoint& );
void put_pixel( Bitmap&, const IntPoint&, const Color& );
void put_pixel_raw( Bitmap&, int x, int y, const Color& );
void replace_color( Bitmap&, const OldColor&, const DrawSource& );
void replace_color_color( Bitmap&, const OldColor&, const NewColor& );
void replace_color_pattern( Bitmap&, const OldColor&, const Bitmap& pattern );
Bitmap rotate( const Bitmap&, radian angle, const Color& background );
Bitmap rotate_90cw( const Bitmap& );
Bitmap scale( const Bitmap&, const Scale&, ScaleQuality );
Bitmap scale_bilinear( const Bitmap&, const Scale& );
Bitmap scale_nearest( const Bitmap&, int scale );
Bitmap scale_nearest( const Bitmap&, const Scale& );
Bitmap scaled_subbitmap( const Bitmap&, const Scale&, const IntRect& );
void set_alpha( Bitmap&, uchar );
void stroke( Bitmap&, const IntPoint& p0, const IntPoint& p1, const Brush&, const Color& );
void stroke( Bitmap&, const std::vector<IntPoint>&, const Brush&, const DrawSource& );
Bitmap sub_bitmap( const Bitmap& src, const IntRect& );

} // namespace faint

#endif
