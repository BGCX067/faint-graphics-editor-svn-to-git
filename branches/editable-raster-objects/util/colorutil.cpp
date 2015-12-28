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

#include "bitmap/bitmap.hh"
#include "rendering/cairocontext.hh" // For cairo_gradient_bitmap
#include "util/colorutil.hh"

namespace faint{

Bitmap alpha_gradient_bitmap( const ColRGB& rgb, const IntSize& size ){
  Bitmap bmp(size);
  const double alphaPerLine = 255.0 / size.h;
  for ( int y = 0; y != size.h; y++ ){
    Color c(rgb, static_cast<faint::uchar>(y * alphaPerLine));
    draw_line_color(bmp, IntPoint(0,y), IntPoint(size.w,y), c, 1, false, LineCap::BUTT);
  }
  return bmp;
}

Bitmap color_bitmap( const Color& color, const IntSize& size, const IntSize& patternSize ){
  ColRGB c1(blend_alpha(color, grayscale_rgb((192))));
  ColRGB c2(blend_alpha(color, grayscale_rgb((255))));

  Bitmap bmp(size);
  clear( bmp, c1 );
  IntSize radius( patternSize / 2 );
  fill_rect_rgb( bmp, IntRect(IntPoint(0,0), size), c1);
  for ( int y = 0; y < size.h; y += patternSize.h ){
    for ( int x = 0; x < size.w; x += patternSize.w ){
      fill_rect_rgb( bmp, IntRect(IntPoint(x,y), radius), c2 );
      fill_rect_rgb( bmp, IntRect(IntPoint(x + radius.w,y + radius.h), radius), c2 );
    }
  }
  return bmp;
}

faint::Bitmap color_bitmap( const faint::Color& color, const IntSize& size ){
  return color_bitmap(color, size, size);
}

Bitmap default_pattern( const IntSize& sz ){
  Bitmap bmp(sz, faint::color_gray());
  int dx = sz.w / 6;
  int dy = sz.w / 2;
  faint::Color fg(0,0,0);
  draw_line_color(bmp, IntPoint(0,dy), IntPoint(dx,dy), fg, 1, false, LineCap::BUTT );

  draw_line_color(bmp, IntPoint(dx, dy), IntPoint(dx * 2,0), fg, 1, false, LineCap::BUTT );
  draw_line_color(bmp, IntPoint(dx, dy), IntPoint(dx * 2,2*dy), fg, 1, false, LineCap::BUTT );

  draw_line_color(bmp, IntPoint(dx * 2, 0), IntPoint(dx * 4, 0), fg, 1, false, LineCap::BUTT );
  draw_line_color(bmp, IntPoint(dx * 2, 2 * dy), IntPoint(dx * 4, 2 * dy), fg, 1, false, LineCap::BUTT );

  draw_line_color(bmp, IntPoint(dx * 4, 0), IntPoint(dx * 5, dy ), fg, 1, false, LineCap::BUTT );
  draw_line_color(bmp, IntPoint(dx * 4, 2 * dy), IntPoint(dx * 5, dy ), fg, 1, false, LineCap::BUTT );

  draw_line_color(bmp, IntPoint(dx * 5, dy), IntPoint(dx * 6, dy ), fg, 1, false, LineCap::BUTT );
  return bmp;
}

faint::Bitmap gradient_bitmap( const faint::Gradient& g, const IntSize& sz ){
  faint::Bitmap bmp(cairo_gradient_bitmap(g, sz));
  faint::Bitmap bg(draw_source_bitmap(faint::DrawSource(faint::color_transparent_white()), sz));
  faint::blend( bmp, onto(bg), IntPoint(0,0));
  return bg;
}

faint::Bitmap gradient_bitmap( const faint::Gradient& g, const IntSize& sz, const IntSize& /*patternSize*/ ){
  return gradient_bitmap( g, sz );
}

Bitmap hue_saturation_color_map( const IntSize& size ){
  Bitmap bmp(size);
  const double centerL = 0.5;

  double hueRate = 360.0 / size.w;
  double saturationRate = 1.0 / size.h;
  for ( int y = 0; y != size.h; y++ ){
    for ( int x = 0; x != size.w; x++ ){
      HSL hsl(x * hueRate, 1.0 - y * saturationRate, centerL);
      put_pixel_raw( bmp, x, y, Color(to_rgb(hsl), 255)  );
    }
  }
  return bmp;
}

Bitmap lightness_gradient_bitmap( const HS& hueSat, const IntSize& size ){
  Bitmap bmp(size);
  double lightnessRate = 1.0 / size.h;
  for ( int y = 0; y != size.h; y++ ){
    Color c( to_rgba( HSL( hueSat, y * lightnessRate ) ) );
    draw_line_color(bmp, IntPoint(0,y), IntPoint(size.w, y), c, 1, false, LineCap::BUTT);
  }
  return bmp;
}

Bitmap with_border( const Bitmap& bmp ){
  Bitmap bmp2(bmp);
  draw_rect_color(bmp2, IntRect(IntPoint(0,0), bmp2.GetSize()), Color(0,0,0), 1, false);
  return bmp2;
}

faint::Bitmap pattern_bitmap( const IntSize& sz ){
  faint::Color bg(220,220,200);
  faint::Color fg(120,120,120);
  faint::Bitmap bmp(sz, bg);
  int dy = sz.h / 3;
  int dx = sz.w / 4;

  // Across
  draw_line_color( bmp, IntPoint(0, dy), IntPoint(sz.w, dy), fg, 1, false, LineCap::BUTT );
  draw_line_color( bmp, IntPoint(0, dy * 2), IntPoint(sz.w, dy * 2), fg, 1, false, LineCap::BUTT );

  // Down
  draw_line_color( bmp, IntPoint(dx * 2, 0), IntPoint(dx * 2, dy), fg, 1, false, LineCap::BUTT );
  draw_line_color( bmp, IntPoint(dx, dy), IntPoint(dx, dy * 2), fg, 1, false, LineCap::BUTT );
  draw_line_color( bmp, IntPoint(dx * 3, dy), IntPoint(dx * 3, dy * 2), fg, 1, false, LineCap::BUTT );
  draw_line_color( bmp, IntPoint(dx * 2, dy * 2), IntPoint(dx * 2, dy * 3), fg, 1, false, LineCap::BUTT );

  return bmp;
}

faint::Bitmap pattern_bitmap( const faint::Pattern&, const IntSize& sz, const IntSize& /*patternSize*/ ){
  return pattern_bitmap(sz);
}

faint::Bitmap draw_source_bitmap( const faint::DrawSource& src, const IntSize& sz ){
  return draw_source_bitmap(src, sz, sz);
}

faint::Bitmap draw_source_bitmap( const faint::DrawSource& src, const IntSize& sz, const IntSize& patternSize ){
  return dispatch(src,
    [&](const faint::Color& c){return color_bitmap(c, sz, patternSize);},
    [&](const faint::Pattern& p){return pattern_bitmap(p, sz, patternSize);},
    [&](const faint::Gradient& g){return gradient_bitmap(g, sz, patternSize);});
}

} // namespace
