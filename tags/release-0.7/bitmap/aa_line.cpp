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

namespace faint{

faint::coord fpart(faint::coord v){
  return v - floated(truncated(v));
}

faint::coord rfpart(faint::coord v){
  return 1 - fpart(v);
}

inline faint::uchar blend_value( faint::uchar oldValue, faint::uchar newValue, double intensity ){
  return static_cast<faint::uchar>(oldValue * (1.0 - intensity) + newValue * intensity);
}

void blend_pixel( Bitmap& bmp, int x, int y, const faint::Color& c, double intensity ){
  int w = static_cast<int>(bmp.m_w);
  int h = static_cast<int>(bmp.m_h);
  if ( x < 0 || y < 0 || x >= w || y >= h ){ // Fixme: Slow, constrain in draw_line_aa_Wu instead
      return;
  }
  faint::Color old(get_color_raw(bmp, x,y));
  put_pixel_raw(bmp, x, y, faint::Color(
    blend_value( old.r, c.r, intensity ),
    blend_value( old.g, c.g, intensity ),
    blend_value( old.b, c.b, intensity ),
    255));
}


void draw_line_aa_Wu(Bitmap& bmp, const IntPoint& in_p0, const IntPoint& in_p1, const Color& color ){
  if ( in_p0 == in_p1 ){
    put_pixel_raw(bmp, in_p0.x, in_p0.y, color);
    return;
  }

  // Reorder coordinates if necessary for simplifying the algorithm
  IntPoint p0(in_p0);
  IntPoint p1(in_p1);
  bool steep = abs(p1.y - p0.y)  > abs(p1.x - p0.x);
  if ( steep ){
    std::swap(p0.x, p0.y);
    std::swap(p1.x, p1.y);
  }
  if ( p0.x > p1.x ){
    std::swap(p0, p1);
  }

  int dx = p1.x - p0.x;
  int dy = p1.y - p0.y;
  const faint::coord gradient = dy / floated(dx);
  faint::coord xend = p0.x;
  faint::coord yend = p0.y + gradient * (xend - p0.x);
  faint::coord xgap = rfpart(p0.x + 0.5);
  int x1 = int(xend);
  int y1 = int(yend);

  // First end point
  if ( steep ){
    blend_pixel(bmp, y1, x1, color, rfpart(yend) * xgap);
    blend_pixel(bmp, y1 + 1, x1, color, fpart(yend) * xgap);
  }
  else{
    blend_pixel(bmp, x1, y1, color, rfpart(yend) * xgap);
    blend_pixel(bmp, x1, y1 + 1, color, fpart(yend) * xgap);
  }
  // First y-intersection for the main-loop
  faint::coord y_inter = yend + gradient;

  // Second end point
  xend = p1.x;
  yend = p1.y + gradient * (xend - p1.x);
  xgap = fpart(p1.x + 0.5);
  int x2 = static_cast<int>(xend);
  int y2 = static_cast<int>(yend);
  if (steep){
    blend_pixel(bmp, y2, x2, color, rfpart(yend) * xgap);
    blend_pixel(bmp, y2 + 1, x2, color, fpart(yend) * xgap);
  }
  else{
    blend_pixel(bmp, x2, y2, color, rfpart(yend) * xgap);
    blend_pixel(bmp, x2, y2+1, color, fpart(yend) * xgap);
  }

  for ( int x = x1 + 1; x != int(x2); x++ ){
    if ( steep ){
      blend_pixel(bmp, int(y_inter), x, color, rfpart(y_inter));
      blend_pixel(bmp, int(y_inter) + 1, x, color, fpart(y_inter));
    }
    else{
      blend_pixel(bmp, x, int(y_inter), color, rfpart(y_inter));
      blend_pixel(bmp, x, int(y_inter) + 1, color, fpart(y_inter));
    }
    y_inter += gradient;
  }
}

} // namespace
