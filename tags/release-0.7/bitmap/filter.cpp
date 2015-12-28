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

#include "bitmap/filter.hh"
#include "rendering/filterclass.hh"
#include "rendering/cairocontext.hh"

template<void Func( faint::Bitmap& )>
class FunctionFilter : public Filter{
public:
  void Apply( faint::Bitmap& bmp ) const{
    Func(bmp);
  }
};

class FilterStroke : public Filter{
  void Apply( faint::Bitmap& bmp ) const{
    faint::Bitmap bg(faint::cairo_compatible_bitmap(bmp.GetSize()));
    clear(bg, faint::Color(255,255,255,0));
    const int width = static_cast<int>(bmp.m_w);
    const int height = static_cast<int>(bmp.m_h);
    for ( int y = 0; y < height; y++ ){
      for ( int x = 0; x < width; x++ ){
	faint::Color current = get_color_raw(bmp, x, y);
	faint::Color right(get_color_raw(bmp, x + 1, y));
	if ( current.a == 0 && right.a != 0 ){
	  faint::fill_ellipse(bg, IntRect(IntPoint(x - 3, y - 3), IntSize(7,7)), faint::Color(0,0,0));
	}
	else if ( current.a != 0 && right.a == 0 ){
	  faint::fill_ellipse(bg, IntRect(IntPoint(x - 3 + 1, y - 3), IntSize(7,7)), faint::Color(0,0,0));
	}

	faint::Color down(get_color_raw(bmp, x, y + 1));
	if ( current.a == 0 && down.a != 0 ){
	  faint::fill_ellipse(bg, IntRect(IntPoint(x - 3, y - 3), IntSize(7,7)), faint::Color(0,0,0));
	}
	if ( current.a != 0 && down.a == 0 ){
	  faint::fill_ellipse(bg, IntRect(IntPoint(x - 3, y - 3 + 1), IntSize(7,7)), faint::Color(0,0,0));
	}
      }
    }
    blit_masked(bmp, onto(bg), faint::Color(255,255,255,0), IntPoint(0,0));
    bmp = bg;
  }
};

namespace faint{

void blur(faint::Bitmap& bmp){
  faint::Bitmap dst(bmp);
  const int width = static_cast<int>(bmp.m_w);
  const int height = static_cast<int>(bmp.m_h);
  for ( int y = 1; y < height; y++ ){
    for ( int x = 1; x < width; x++ ){
      faint::Color a0 = get_color_raw(bmp, x-1,y-1);
      faint::Color a1 = get_color_raw(bmp, x ,y-1);
      faint::Color a2 = get_color_raw(bmp, x+1 ,y-1);
      faint::Color b0 = get_color_raw(bmp, x-1,y);
      faint::Color b1 = get_color_raw(bmp, x ,y);
      faint::Color b2 = get_color_raw(bmp, x+1 ,y);
      faint::Color c0 = get_color_raw(bmp, x-1,y+1);
      faint::Color c1 = get_color_raw(bmp, x ,y+1);
      faint::Color c2 = get_color_raw(bmp, x+1 ,y+1);

      faint::uchar r = static_cast<faint::uchar>(
        ( int(a0.r) * 1 + int(a1.r) * 2 + int(a2.r) * 1 +
	  int(b0.r) * 2 + int(b1.r) * 4 + int(b2.r) * 2 +
	  int(c0.r) * 1 + int(c1.r) * 2 + int(c2.r) * 1) / 16 );
      faint::uchar g = static_cast<faint::uchar>(
        ( int(a0.g) * 1 + int(a1.g) * 2 + int(a2.g) * 1 +
	  int(b0.g) * 2 + int(b1.g) * 4 + int(b2.g) * 2 +
	  int(c0.g) * 1 + int(c1.g) * 2 + int(c2.g) * 1) / 16 );
      faint::uchar b = static_cast<faint::uchar>(
        ( int(a0.b) * 1 + int(a1.b) * 2 + int(a2.b) * 1 +
	  int(b0.b) * 2 + int(b1.b) * 4 + int(b2.b) * 2 +
	  int(c0.b) * 1 + int(c1.b) * 2 + int(c2.b) * 1) / 16 );
      faint::uchar a = static_cast<faint::uchar>(
        (int(a0.a) * 1 + int(a1.a) * 2 + int(a2.a) * 1 +
	  int(b0.a) * 2 + int(b1.a) * 4 + int(b2.a) * 2 +
	  int(c0.a) * 1 + int(c1.a) * 2 + int(c2.a) * 1) / 16 );

      put_pixel_raw(dst, x,y, faint::Color(r,g,b,a));
    }
  }
  bmp = dst;
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

Filter* get_blur_filter(){
  return new FunctionFilter<blur>();
}

Filter* get_stroke_filter(){
  return new FilterStroke();
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

} // namespace
