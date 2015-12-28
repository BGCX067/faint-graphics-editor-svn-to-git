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

#include <algorithm>
#include <cassert>
#include <functional> // std::bind
#include "bitmap/bitmap-templates.hh"
#include "bitmap/filter.hh"
#include "geo/angle.hh"
#include "geo/measure.hh"
#include "geo/padding.hh"
#include "geo/point.hh"
#include "geo/rotation.hh"
#include "rendering/filter-class.hh"
#include "util/color.hh"
#include "util/gauss.hh"

namespace faint{

template<int PADDING, void Func(Bitmap&)>
class FunctionFilter : public Filter{
public:
  void Apply(Bitmap& bmp) const override{
    Func(bmp);
  }

  Padding GetPadding() const{
    return Padding::All(PADDING);
  }
};

class FilterStroke : public Filter{
public:
  void Apply(Bitmap& bmp) const override{
    // Fixme: Allocation, handle OOM
    Bitmap bg(bmp.GetSize(), color_transparent_white());
    for (int y = 1; y < bmp.m_h - 1; y++){
      for (int x = 1; x < bmp.m_w - 1; x++){
        Color current = get_color_raw(bmp, x, y);
        Color right(get_color_raw(bmp, x + 1, y));
        if (current.a == 0 && right.a != 0){
          fill_ellipse_color(bg, IntRect(IntPoint(x - 3, y - 3), IntSize(7,7)), Color(0,0,0));
        }
        else if (current.a != 0 && right.a == 0){
          fill_ellipse_color(bg, IntRect(IntPoint(x - 3 + 1, y - 3), IntSize(7,7)), Color(0,0,0));
        }

        Color down(get_color_raw(bmp, x, y + 1));
        if (current.a == 0 && down.a != 0){
          fill_ellipse_color(bg, IntRect(IntPoint(x - 3, y - 3), IntSize(7,7)), Color(0,0,0));
        }
        if (current.a != 0 && down.a == 0){
          fill_ellipse_color(bg, IntRect(IntPoint(x - 3, y - 3 + 1), IntSize(7,7)), Color(0,0,0));
        }
      }
    }
    blit_masked(bmp, onto(bg), color_transparent_white(), IntPoint(0,0));
    bmp = bg;
  }

  Padding GetPadding() const{
    return Padding::All(5);
  }
};

static void increase_alpha(Bitmap& bmp, int x, int y, int d){
  Color c = get_color_raw(bmp, x, y);
  c.a = static_cast<uchar>(std::min(static_cast<int>(c.a) + d, 255));
  put_pixel_raw(bmp, x, y, c);
}

class FilterShadow : public Filter{
public:
  int m_dist;
  FilterShadow(){
    m_dist = 9;
  }
  void Apply(Bitmap& bmp) const override{
    // Fixme: Allocation, handle OOM
    Bitmap bg(bmp.GetSize(), color_transparent_black());
    const int c1 = 20;
    const int c2 = 15;
    const int c3 = 10;
    const int c4 = 5;
    for (int y = 0; y < bmp.m_h - m_dist - 5; y++){
      for (int x = 0; x < bmp.m_w - m_dist - 5; x++){
        if (get_color_raw(bmp, x, y).a != 0){
          int x2 = x + m_dist;
          int y2 = y + m_dist;
          increase_alpha(bg, x2 + 1, y2, c4);
          increase_alpha(bg, x2 + 2, y2, c3);
          increase_alpha(bg, x2 + 3, y2, c4);

          increase_alpha(bg, x2, y2 + 1, c4);
          increase_alpha(bg, x2 + 1, y2 + 1, c3);
          increase_alpha(bg, x2 + 2, y2 + 1, c2);
          increase_alpha(bg, x2 + 3, y2 + 1, c3);
          increase_alpha(bg, x2 + 4, y2 + 1, c4);

          increase_alpha(bg, x2, y2 + 2, c3);
          increase_alpha(bg, x2 + 1, y2 + 2, c2);
          increase_alpha(bg, x2 + 2, y2 + 2, c1);
          increase_alpha(bg, x2 + 3, y2 + 2, c2);
          increase_alpha(bg, x2 + 4, y2 + 2, c3);

          increase_alpha(bg, x2, y2 + 3, c4);
          increase_alpha(bg, x2 + 1, y2 + 3, c3);
          increase_alpha(bg, x2 + 2, y2 + 3, c2);
          increase_alpha(bg, x2 + 3, y2 + 3, c3);
          increase_alpha(bg, x2 + 4, y2 + 3, c4);

          increase_alpha(bg, x2 + 1, y2 + 4, c4);
          increase_alpha(bg, x2 + 2, y2 + 4, c3);
          increase_alpha(bg, x2 + 3, y2 + 4, c4);
        }
      }
    }

    blit_masked(bmp, onto(bg), color_transparent_white(), IntPoint(0,0));
    bmp = bg;
  }

  Padding GetPadding() const{
    return Padding::Right(6 + m_dist) + Padding::Bottom(6 + m_dist);
  }
};

brightness_contrast_t::brightness_contrast_t()
  : brightness(0.0),
    contrast(1.0)
{}

brightness_contrast_t::brightness_contrast_t(double in_brightness, double in_contrast)
  : brightness(in_brightness),
    contrast(in_contrast)
{}

void blur(Bitmap& bmp){
  Bitmap dst(copy(bmp));
  for (int y = 1; y < bmp.m_h - 1; y++){
    for (int x = 1; x < bmp.m_w - 1; x++){
      Color a0 = get_color_raw(bmp, x-1,y-1);
      Color a1 = get_color_raw(bmp, x ,y-1);
      Color a2 = get_color_raw(bmp, x+1 ,y-1);
      Color b0 = get_color_raw(bmp, x-1,y);
      Color b1 = get_color_raw(bmp, x ,y);
      Color b2 = get_color_raw(bmp, x+1 ,y);
      Color c0 = get_color_raw(bmp, x-1,y+1);
      Color c1 = get_color_raw(bmp, x ,y+1);
      Color c2 = get_color_raw(bmp, x+1 ,y+1);

      uchar r = static_cast<uchar>(
        (int(a0.r) * 1 + int(a1.r) * 2 + int(a2.r) * 1 +
          int(b0.r) * 2 + int(b1.r) * 4 + int(b2.r) * 2 +
          int(c0.r) * 1 + int(c1.r) * 2 + int(c2.r) * 1) / 16);
      uchar g = static_cast<uchar>(
        (int(a0.g) * 1 + int(a1.g) * 2 + int(a2.g) * 1 +
          int(b0.g) * 2 + int(b1.g) * 4 + int(b2.g) * 2 +
          int(c0.g) * 1 + int(c1.g) * 2 + int(c2.g) * 1) / 16);
      uchar b = static_cast<uchar>(
        (int(a0.b) * 1 + int(a1.b) * 2 + int(a2.b) * 1 +
          int(b0.b) * 2 + int(b1.b) * 4 + int(b2.b) * 2 +
          int(c0.b) * 1 + int(c1.b) * 2 + int(c2.b) * 1) / 16);
      uchar a = static_cast<uchar>(
        (int(a0.a) * 1 + int(a1.a) * 2 + int(a2.a) * 1 +
          int(b0.a) * 2 + int(b1.a) * 4 + int(b2.a) * 2 +
          int(c0.a) * 1 + int(c1.a) * 2 + int(c2.a) * 1) / 16);
      put_pixel_raw(dst, x,y, Color(r,g,b,a));
    }
  }
  bmp = dst;
}

void apply_brightness_and_contrast(Bitmap& bmp, const brightness_contrast_t& v){
  double scaledBrightness = v.brightness * 255.0;
  for (int y = 0; y != bmp.m_h; y++){
    for (int x = 0; x != bmp.m_w; x++){
      Color c(get_color_raw(bmp, x, y));
      Color c2(color_from_double(c.r * v.contrast + scaledBrightness,
          c.g * v.contrast + scaledBrightness,
          c.b * v.contrast + scaledBrightness));
      put_pixel_raw(bmp, x, y, c2);
    }
  }
}

void desaturate_simple(Bitmap& bmp){
  uchar* data = bmp.m_data;
  for (int y = 0; y != bmp.m_h; y++){
    for (int x = 0; x != bmp.m_w; x++){
      int dst = y * bmp.m_row_stride + x * 4;
      uchar gray = static_cast<uchar>((data[dst + iR ] + data[dst + iG ] + data[dst + iB ]) / 3);
      data[dst + iR] = gray;
      data[dst + iG] = gray;
      data[dst + iB] = gray;
    }
  }
}

void desaturate_weighted(Bitmap& bmp){
  uchar* data = bmp.m_data;
  for (int y = 0; y != bmp.m_h; y++){
    for (int x = 0; x != bmp.m_w; x++){
      int dst = y * bmp.m_row_stride + x * 4;
      uchar gray = static_cast<uchar>((0.3 * data[dst + iR ] + 0.59 * data[dst + iG ] + 0.11 * data[dst + iB ]));
      data[dst + iR ] = gray;
      data[dst + iG ] = gray;
      data[dst + iB ] = gray;
    }
  }
}

static Bitmap kernel_h_apply(const Bitmap& src, const std::vector<double>& k){
  Bitmap dst(src.GetSize());
  int r = static_cast<int>(k.size()) / 2;
  int w = resigned(k.size());
  const int bpp = 4;
  for (int y = 0; y != src.m_h; y++){
    const uchar* srcRow = src.m_data + y * src.m_row_stride;
    uchar* dstRow = dst.m_data + y * dst.m_row_stride;

    for (int x = 0; x != src.m_w; x++){
      double vR = 0;
      double vG = 0;
      double vB = 0;
      double vA = 0;

      for (int i = 0; i != w; i++){
        int d = std::min(x, std::max(i - r, -(src.m_w - x - 1)));
        vR += srcRow[(x - d) * bpp + iR] * k[to_size_t(i)];
        vG += srcRow[(x - d) * bpp + iG] * k[to_size_t(i)];
        vB += srcRow[(x - d) * bpp + iB] * k[to_size_t(i)];
        vA += srcRow[(x - d) * bpp + iA] * k[to_size_t(i)];
      }
      dstRow[x * 4 + iR] = static_cast<uchar>(vR);
      dstRow[x * 4 + iG] = static_cast<uchar>(vG);
      dstRow[x * 4 + iB] = static_cast<uchar>(vB);
      dstRow[x * 4 + iA] = static_cast<uchar>(vA);
    }
  }
  return dst;
}

static Bitmap kernel_v_apply(const Bitmap& src, const std::vector<double>& k){
  Bitmap dst(src.GetSize());
  int r = static_cast<int>(k.size()) / 2;
  int w = resigned(k.size());

  for (int y = 0; y != src.m_h; y++){
    const uchar* srcRow = src.m_data + y * src.m_row_stride;
    uchar* dstRow = dst.m_data + y * dst.m_row_stride;
    for (int x = 0; x != src.m_w; x++){
      double vR = 0;
      double vG = 0;
      double vB = 0;
      double vA = 0;
      for (int i = 0; i != w; i++){
        const int d = std::min(y, std::max(i - r, -(src.m_h - y - 1)));
        vR += srcRow[x * 4 - d * src.m_row_stride + iR] * k[to_size_t(i)];
        vG += srcRow[x * 4 - d * src.m_row_stride + iG] * k[to_size_t(i)];
        vB += srcRow[x * 4 - d * src.m_row_stride + iB] * k[to_size_t(i)];
        vA += srcRow[x * 4 - d * src.m_row_stride + iA] * k[to_size_t(i)];
      }

      dstRow[x * 4 + iR] = static_cast<uchar>(vR);
      dstRow[x * 4 + iG] = static_cast<uchar>(vG);
      dstRow[x * 4 + iB] = static_cast<uchar>(vB);
      dstRow[x * 4 + iA] = static_cast<uchar>(vA);
    }
  }
  return dst;
}

static Bitmap gaussian_blurred(const Bitmap& src, double sigma){
  auto k = normalized_gauss_kernel_1d(sigma);
  return kernel_v_apply(kernel_h_apply(src, k), k);
}

void gaussian_blur(Bitmap& bmp, double sigma){
  bmp = gaussian_blurred(bmp, sigma);
}

void pixelize(Bitmap& bmp, int w){
  assert(w >= 1);
  IntSize sz(bmp.GetSize());
  Bitmap bmp2(sz);
  for (int y = 0; y < sz.h; y +=w){
    for (int x = 0; x < sz.w; x +=w){
      int r = 0;
      int g = 0;
      int b = 0;
      int a = 0;
      int count = 0;
      for (int j = 0; y + j != sz.h && j != w; j++){
        for (int i = 0; x + i != sz.w && i != w; i++){
          Color c = get_color_raw(bmp, x + i, y + j);
          r += c.r;
          g += c.g;
          b += c.b;
          a += c.a;
          count++;
        }
      }

      Color c2(color_from_ints(r / count, g / count, b / count, a / count));
      for (int j = 0; y + j < sz.h && j != w; j++){
        for (int i = 0; x + i != sz.w && i != w; i++){
          put_pixel_raw(bmp2, x + i, y + j, c2);
        }
      }
    }
  }
  bmp = bmp2;
}

void sepia(Bitmap& bmp, int intensity){
  // Modified from https://groups.google.com/forum/#!topic/comp.lang.java.programmer/nSCnLECxGdA
  int depth = 20;
  const IntSize sz = bmp.GetSize();

  for (int y = 0; y != sz.h; y++){
    for (int x = 0; x != sz.w; x++){
      Color c = get_color_raw(bmp, x, y);
      const int gray = (c.r + c.g + c.b) / 3;
      const int r = std::min(gray + depth * 2, 255);
      const int g = std::min(gray + depth, 255);
      const int b = std::max(gray - intensity, 0);
      put_pixel_raw(bmp, x, y, color_from_ints(r,g,b, c.a));
    }
  }
}

int color_sum(const Color& c){
  return c.r + c.g + c.b;
}

void threshold(Bitmap& bmp, const threshold_range_t& range, const Paint& in, const Paint& out){
  const Interval interval(range.GetInterval());
  set_pixels_if_else(bmp, in, out,
    [&interval, &bmp](int x, int y){
      return interval.Has(color_sum(get_color_raw(bmp, x, y)));
    }
 );
}

std::vector<int> threshold_histogram(Bitmap& bmp){
  std::vector<int> v(766, 0);
  for (int y = 0; y != bmp.m_h; y++){
    for (int x = 0; x != bmp.m_w; x++){
      Color c(get_color_raw(bmp, x, y));
      v[c.r + c.g + c.b] += 1;
    }
  }
  return v;
}

Bitmap subtract(const Bitmap& lhs, const Bitmap& rhs){
  assert(lhs.GetSize() == rhs.GetSize());
  Bitmap dst(lhs.GetSize());
  for (int y = 0; y != lhs.m_h; y++){
    for (int x = 0; x != lhs.m_w; x++){
      put_pixel_raw(dst,
        x, y, subtract(get_color_raw(lhs, x, y), get_color_raw(rhs, x, y)));
    }
  }
  return dst;
}

Bitmap add(const Bitmap& lhs, const Bitmap& rhs){
  assert(lhs.GetSize() == rhs.GetSize());
  Bitmap dst(lhs.GetSize());
  for (int y = 0; y != lhs.m_h; y++){
    for (int x = 0; x != lhs.m_w; x++){
      put_pixel_raw(dst,
        x, y, add(get_color_raw(lhs, x, y), get_color_raw(rhs, x, y)));
    }
  }
  return dst;
}

void unsharp_mask(Bitmap& bmp, double blurSigma){
  Bitmap blurred = gaussian_blurred(bmp, blurSigma);
  Bitmap usm = subtract(bmp, blurred);
  bmp = add(bmp, usm);
}

std::vector<int> red_histogram(Bitmap& bmp){
  std::vector<int> v(256,0);
  for (int y = 0; y != bmp.m_h; y++){
    for (int x = 0; x != bmp.m_w; x++){
      Color c(get_color_raw(bmp, x, y));
      v[c.r] += 1;
    }
  }
  return v;
}

std::vector<int> green_histogram(Bitmap& bmp){
  std::vector<int> v(256,0);
  for (int y = 0; y != bmp.m_h; y++){
    for (int x = 0; x != bmp.m_w; x++){
      Color c(get_color_raw(bmp, x, y));
      v[c.g] += 1;
    }
  }
  return v;
}

std::vector<int> blue_histogram(Bitmap& bmp){
  std::vector<int> v(256,0);
  for (int y = 0; y != bmp.m_h; y++){
    for (int x = 0; x != bmp.m_w; x++){
      Color c(get_color_raw(bmp, x, y));
      v[c.b] += 1;
    }
  }
  return v;
}

Filter* get_blur_filter(){
  return new FunctionFilter<5, blur>();
}

Filter* get_invert_filter(){
  return new FunctionFilter<0, invert>();
}

Filter* get_stroke_filter(){
  return new FilterStroke();
}

Filter* get_shadow_filter(){
  return new FilterShadow();
}

void pixelize_5(Bitmap& bmp){
  pixelize(bmp, 5);
}

Filter* get_pixelize_filter(){
  using namespace std::placeholders;
  return new FunctionFilter<0, pixelize_5>();
}

void invert(Bitmap& bmp){
  for (int y = 0; y != bmp.m_h; y++){
    uchar* data = bmp.m_data + y * bmp.m_row_stride;
    for (int x = 0; x != bmp.m_w * 4; x += 4){
      uchar* pos = data + x;
      *(pos + iR) = static_cast<uchar>(255 - *(pos + iR));
      *(pos + iG) = static_cast<uchar>(255 - *(pos + iG));
      *(pos + iB) = static_cast<uchar>(255 - *(pos + iB));
    }
  }
}

static uchar clip_rgb(coord value){
  return static_cast<uchar>(std::min(255.0, std::max(0.0, value)));
}

void color_balance(Bitmap& bmp, const color_range_t& r, const color_range_t& g, const color_range_t& b){

  const Interval rSpan(r.GetInterval());
  const Interval gSpan(b.GetInterval());
  const Interval bSpan(g.GetInterval());
  double Ar = static_cast<coord>(rSpan.Lower());
  double Br = static_cast<coord>(rSpan.Upper());
  double Ag = static_cast<coord>(gSpan.Lower());
  double Bg = static_cast<coord>(gSpan.Upper());
  double Ab = static_cast<coord>(bSpan.Lower());
  double Bb = static_cast<coord>(bSpan.Upper());
  double L = 0.0;
  double U = 255.0;
  double Xr = (U-L) / (Br-Ar);
  double Yr = L - Ar*((U-L)/ (Br-Ar));
  double Xg = (U-L) / (Bg-Ag);
  double Yg = L - Ag*((U-L)/ (Bg-Ag));
  double Xb = (U-L) / (Bb-Ab);
  double Yb = L - Ab*((U-L)/ (Bb-Ab));

  for (int y = 0; y != bmp.m_h; y++){
    uchar* row = bmp.m_data + y * bmp.m_row_stride;
    for (int x = 0; x != bmp.m_w * 4; x += 4){
      uchar* pos = row + x;
      *(pos + iR) = clip_rgb(*(pos + iR) * Xr + Yr);
      *(pos + iG) = clip_rgb(*(pos + iG) * Xg + Yg);
      *(pos + iB) = clip_rgb(*(pos + iB) * Xb + Yb);
    }
  }
}

template<typename T>
T sq(T v){
  return v*v;
}

static IntPoint whirled(const Point& p, const Point& c, coord r, const Rotation& whirl){
  const Rotation angle = whirl * sq(1.0 - r);
  coord sina = sin(angle);
  coord cosa = cos(angle);
  return IntPoint(truncated(cosa * p.x - sina * p.y + c.x),
    truncated(sina * p.x + cosa * p.y + c.y));
}

void filter_pinch_whirl(Bitmap& bmp, coord pinch, const Rotation& whirl){
  Bitmap bmpOld(copy(bmp));
  Point c((bmp.m_w - 1) / 2.0, (bmp.m_h - 1) / 2.0);
  coord r2 = sq(bmp.m_w / 2.0);

  for (int y = 0; y != bmp.m_h; y++){
    for (int x = 0; x != bmp.m_w; x++){
      Point delta(x - c.x, y - c.y);
      coord d = sq(delta.x) + sq(delta.y);

      if (d < r2){
        coord dist = sqrt(d / 1.0) / (bmp.m_w / 2.0);
        delta *= pow(sin(pi/2*dist), pinch);
        IntPoint p2 = whirled(delta, c, dist, whirl);
        if (p2.x > 0 && p2.y > 0 && p2.x < bmp.m_w && p2.y < bmp.m_h){
          put_pixel_raw(bmp, x, y, get_color(bmpOld, p2));
        }
      }
    }
  }
}

void filter_pinch_whirl_forward(Bitmap& bmp){
  filter_pinch_whirl(bmp, 0.5, Rotation::Rad(0.5));
}


Filter* get_pinch_whirl_filter(){
  return new FunctionFilter<10, filter_pinch_whirl_forward>();
}

} // namespace
