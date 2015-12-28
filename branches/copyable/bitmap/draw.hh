// -*- coding: us-ascii-unix -*-
// Copyright 2014 Lukas Kemmer
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

#ifndef FAINT_DRAW_HH
#define FAINT_DRAW_HH
#include "util/common-fwd.hh"
#include "util/setting-id.hh"

namespace faint{
template<typename T> class Offsat;
typedef Order<faint::Color>::New NewColor;
typedef Order<faint::Color>::Old OldColor;

class category_axiality;
typedef Distinct<Axis, category_axiality, 0> along;
typedef Distinct<Axis, category_axiality, 1> across;

class DstBmp;

class BorderSettings{
public:
  BorderSettings(const Paint& paint, int lineWidth, bool dashes) :
    paint(paint), lineWidth(lineWidth), dashes(dashes)
  {}

  BorderSettings(const Paint&& paint, int lineWidth, bool dashes) :
    paint(std::move(paint)), lineWidth(lineWidth), dashes(dashes)
  {}

  BorderSettings(const Color& color, int lineWidth, bool dashes) :
    paint(color), lineWidth(lineWidth), dashes(dashes)
  {}

  Paint paint;
  int lineWidth;
  bool dashes;
};

class LineSettings{
public:
  LineSettings(const Paint& p, int lineWidth, bool dashes, LineCap cap)
    : paint(p), lineWidth(lineWidth), dashes(dashes), cap(cap)
  {}

  LineSettings(const Color& c, int lineWidth, bool dashes, LineCap cap)
    : paint(c), lineWidth(lineWidth), dashes(dashes), cap(cap)
  {}

  LineSettings(const BorderSettings& b, LineCap cap)
    : paint(b.paint), lineWidth(b.lineWidth), dashes(b.dashes), cap(cap)
  {}

  LineSettings(const BorderSettings&& b, LineCap cap)
    : paint(std::move(b.paint)), lineWidth(b.lineWidth), dashes(b.dashes), cap(cap)
  {}
  Paint paint;
  int lineWidth;
  bool dashes;
  LineCap cap;
};

Bitmap alpha_blended(const Bitmap&, const Color&);
Bitmap alpha_blended(const Bitmap&, const ColRGB&);
void blend_alpha(Bitmap&, const ColRGB&);
void blend(const Bitmap& src, DstBmp, const IntPoint& topLeft);
void blend_masked(const Bitmap& src, DstBmp, const Color& maskColor, const IntPoint& topLeft);
void blend(Bitmap&, const Offsat<AlphaMapRef>&, const Paint&);
void blit(const Bitmap&, DstBmp, const IntPoint& topLeft);
void blit_masked(const Bitmap& src, DstBmp, const Color&, const IntPoint& topLeft);
void boundary_fill(Bitmap&, const IntPoint&, const Paint& fill, const Color& boundaryColor);
Brush circle_brush(int w);
void draw_ellipse(Bitmap&, const IntRect&, const BorderSettings&);
void draw_line(Bitmap&, const IntLineSegment&, const LineSettings&);
void draw_polygon(Bitmap&, const std::vector<IntPoint>&, const BorderSettings&);
void draw_polyline(Bitmap&, const std::vector<IntPoint>&, const LineSettings&);
void draw_rect(Bitmap&, const IntRect&, const BorderSettings&);
void erase_but(Bitmap&, const Color& keep, const Paint& eraser);
void fill_ellipse(Bitmap&, const IntRect&, const Paint&);
void fill_ellipse_color(Bitmap&, const IntRect&, const Color&);
void fill_polygon(Bitmap&, const std::vector<IntPoint>&, const Paint&);
void fill_rect(Bitmap&, const IntRect&, const Paint&);
void fill_rect_color(Bitmap&, const IntRect&, const Color&);
void fill_rect_rgb(Bitmap&, const IntRect&, const ColRGB&);
void fill_triangle(Bitmap&, const Point&, const Point&, const Point&, const Paint&);
void fill_triangle_color(Bitmap&, const Point&, const Point&, const Point&, const Color&);
Bitmap flip(const Bitmap&, const along&);
Bitmap flip(const Bitmap&, const across&);
void flood_fill_color(Bitmap&, const IntPoint&, const Color& fillColor);
void flood_fill(Bitmap&, const IntPoint&, const Paint&);
std::vector<Color> get_palette(const Bitmap&);
bool overextends_down(const IntRect&, const Bitmap&);
bool overextends_left(const IntRect&, const Bitmap&);
bool overextends_right(const IntRect&, const Bitmap&);
bool overextends_up(const IntRect&, const Bitmap&);
void put_pixel(Bitmap&, const IntPoint&, const Color&);
void put_pixel_raw(Bitmap&, int x, int y, const Color&);
void replace_color(Bitmap&, const OldColor&, const Paint&);
void replace_color_color(Bitmap&, const OldColor&, const NewColor&);
void replace_color_pattern(Bitmap&, const OldColor&, const Bitmap& pattern);
void rect(Bitmap&, const IntRect&, const Optional<BorderSettings>&, const Optional<Paint>& bg);
Bitmap rotate_nearest(const Bitmap&, const Angle&, const Color& bg);
Bitmap rotate_bilinear(const Bitmap&, const Angle&, const Paint& bg);
Bitmap rotate_bilinear(const Bitmap&, const Angle&);
Bitmap rotate_90cw(const Bitmap&);
Bitmap rotate_scale_bilinear(const Bitmap&, const Angle&, const Scale&, const Paint& bg);
Bitmap scale(const Bitmap&, const Scale&, ScaleQuality);
Bitmap scale_bilinear(const Bitmap&, const Scale&);
Bitmap scale_nearest(const Bitmap&, int scale);
Bitmap scale_nearest(const Bitmap&, const Scale&);
Bitmap scaled_subbitmap(const Bitmap&, const Scale&, const IntRect&);
void set_alpha(Bitmap&, uchar);
Bitmap subbitmap(const Bitmap&, const IntRect&);
} // namespace faint

#endif
