// -*- coding: us-ascii-unix -*-
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
#include "bitmap/draw.hh"
#include "geo/intrect.hh"
#include "geo/line.hh"
#include "rendering/cairo-context.hh" // For cairo_gradient_bitmap
#include "util/color.hh"
#include "util/color-bitmap-util.hh"

namespace faint{

Bitmap alpha_gradient_bitmap(const ColRGB& rgb, const IntSize& size){
  Bitmap bmp(size);
  const double alphaPerLine = 255.0 / size.h;
  for (int y = 0; y != size.h; y++){
    Color c(rgb, static_cast<uchar>(y * alphaPerLine));
    draw_line(bmp, {{0,y}, {size.w,y}}, {c, 1, false, LineCap::BUTT});
  }
  return bmp;
}

Bitmap color_bitmap(const Color& color, const IntSize& size, const IntSize& patternSize){
  ColRGB c1(blend_alpha(color, grayscale_rgb((192))));
  ColRGB c2(blend_alpha(color, grayscale_rgb((255))));

  Bitmap bmp(size);
  clear(bmp, c1);
  IntSize radius(patternSize / 2);
  fill_rect_rgb(bmp, IntRect(IntPoint(0,0), size), c1);
  for (int y = 0; y < size.h; y += patternSize.h){
    for (int x = 0; x < size.w; x += patternSize.w){
      fill_rect_rgb(bmp, IntRect(IntPoint(x,y), radius), c2);
      fill_rect_rgb(bmp, IntRect(IntPoint(x + radius.w,y + radius.h), radius), c2);
    }
  }
  return bmp;
}

Bitmap color_bitmap(const Color& color, const IntSize& size){
  return color_bitmap(color, size, size);
}

Bitmap default_pattern(const IntSize& sz){
  Bitmap bmp(sz, color_gray());
  int dx = sz.w / 6;
  int dy = sz.w / 2;
  LineSettings s(Color(0,0,0), 1, false, LineCap::BUTT);
  draw_line(bmp, {{0,dy}, {dx,dy}}, s);

  draw_line(bmp, {{dx, dy}, {dx * 2, 0}}, s);
  draw_line(bmp, {{dx, dy}, {dx * 2,2*dy}}, s);

  draw_line(bmp, {{dx * 2, 0}, {dx * 4, 0}}, s);
  draw_line(bmp, {{dx * 2, 2 * dy}, {dx * 4, 2 * dy}}, s);

  draw_line(bmp, {{dx * 4, 0}, {dx * 5, dy}}, s);
  draw_line(bmp, {{dx * 4, 2 * dy}, {dx * 5, dy}}, s);

  draw_line(bmp, {{dx * 5, dy}, {dx * 6, dy}}, s);
  return bmp;
}

Bitmap gradient_bitmap(const Gradient& g, const IntSize& sz){
  Bitmap bmp(cairo_gradient_bitmap(g, sz));
  Bitmap bg(paint_bitmap(Paint(color_transparent_white()), sz));
  blend(bmp, onto(bg), IntPoint(0,0));
  return bg;
}

Bitmap gradient_bitmap(const Gradient& g, const IntSize& sz, const IntSize& /*patternSize*/){
  return gradient_bitmap(g, sz);
}

Bitmap hue_saturation_color_map(const IntSize& size){
  Bitmap bmp(size);
  const double centerL = 0.5;

  double hueRate = 360.0 / size.w;
  double saturationRate = 1.0 / size.h;
  for (int y = 0; y != size.h; y++){
    for (int x = 0; x != size.w; x++){
      HSL hsl(x * hueRate, 1.0 - y * saturationRate, centerL);
      put_pixel_raw(bmp, x, y, Color(to_rgb(hsl), 255) );
    }
  }
  return bmp;
}

Bitmap lightness_gradient_bitmap(const HS& hueSat, const IntSize& size){
  Bitmap bmp(size);
  double lightnessRate = 1.0 / size.h;
  for (int y = 0; y != size.h; y++){
    Color c(to_rgba(HSL(hueSat, y * lightnessRate)));
    draw_line(bmp, {{0,y}, {size.w, y}}, {c, 1, false, LineCap::BUTT});
  }
  return bmp;
}

Bitmap with_border(const Bitmap& bmp, bool dashed){
  return with_border(bmp, color_black(), dashed);
}

Bitmap with_border(const Bitmap& bmp, const Color& c, bool dashed){
  Bitmap bmp2(copy(bmp));
  draw_rect(bmp2, IntRect(IntPoint(0,0), bmp2.GetSize()), {c, 1, dashed});
  return bmp2;
}

Bitmap pattern_bitmap(const IntSize& sz){
  Color bg(220,220,200);
  Color fg(120,120,120);
  Bitmap bmp(sz, bg);
  int dy = sz.h / 3;
  int dx = sz.w / 4;
  const LineSettings s(fg, 1, false, LineCap::BUTT);
  // Across
  draw_line(bmp, {{0, dy}, {sz.w, dy}}, s);
  draw_line(bmp, {{0, dy * 2}, {sz.w, dy * 2}}, s);

  // Down
  draw_line(bmp, {{dx * 2, 0}, {dx * 2, dy}}, s);
  draw_line(bmp, {{dx, dy}, {dx, dy * 2}}, s);
  draw_line(bmp, {{dx * 3, dy}, {dx * 3, dy * 2}}, s);
  draw_line(bmp, {{dx * 2, dy * 2}, {dx * 2, dy * 3}}, s);

  return bmp;
}

Bitmap pattern_bitmap(const Pattern&, const IntSize& sz, const IntSize& /*patternSize*/){
  return pattern_bitmap(sz);
}

Bitmap paint_bitmap(const Paint& paint, const IntSize& sz){
  return paint_bitmap(paint, sz, sz);
}

Bitmap paint_bitmap(const Paint& paint, const IntSize& sz, const IntSize& patternSize){
  return dispatch(paint,
    [&](const Color& c){return color_bitmap(c, sz, patternSize);},
    [&](const Pattern& p){return pattern_bitmap(p, sz, patternSize);},
    [&](const Gradient& g){return gradient_bitmap(g, sz, patternSize);});
}

} // namespace
