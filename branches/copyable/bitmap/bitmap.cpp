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

#include <cassert>
#include <cstring> // memcpy
#include "bitmap/bitmap.hh"
#include "bitmap/bitmap-templates.hh"
#include "geo/intrect.hh"
#include "rendering/cairo-context.hh" // faint_cairo_stride
#include "util/color.hh"
#include "util/pattern.hh"

namespace faint{
int faint_cairo_stride(const IntSize&);

const uint bpp = 4;
const int iR = 2;
const int iG = 1;
const int iB = 0;
const int iA = 3;

static void clear_pattern(Bitmap& bmp, const Pattern& pattern){
  const IntSize sz(bmp.GetSize());
  const Bitmap& srcBmp(pattern.GetBitmap());
  for (int y = 0; y != sz.h; y++){
    for (int x = 0; x != sz.w; x++){
      put_pixel_raw(bmp, x, y, get_color_modulo_raw(srcBmp, x, y));
    }
  }
}

Bitmap::Bitmap()
  : m_row_stride(0)
{
  m_w = m_h = 0;
  m_data = nullptr;
}

Bitmap::Bitmap(const CopySrc<Bitmap>& other)
  : m_row_stride(other.obj.m_row_stride),
    m_w(other.obj.m_w),
    m_h(other.obj.m_h),
    m_data(nullptr)
{
  if (other.obj.m_data != nullptr){
    m_data = new uchar[ m_row_stride * m_h ];
    memcpy(m_data, other.obj.m_data, to_size_t(m_row_stride * m_h));
  }
}

Bitmap::Bitmap(const IntSize& sz)
  : m_row_stride(0),
    m_w(sz.w),
    m_h(sz.h),
    m_data(nullptr)
{
  assert(m_w > 0);
  assert(m_h > 0);
  m_row_stride = faint_cairo_stride(sz);
  if (m_row_stride == -1){
    throw std::bad_alloc(); // Fixme
  }
  m_data = new uchar[ m_row_stride * m_h ];
  memset(m_data, 0, to_size_t(m_row_stride * m_h));
}

Bitmap::Bitmap(const IntSize& sz, const Color& bgColor)
  : m_row_stride(0),
    m_w(sz.w),
    m_h(sz.h),
    m_data(nullptr)
{
  assert(m_w > 0);
  assert(m_h > 0);
  m_row_stride = faint_cairo_stride(sz);
  if (m_row_stride == -1){
    throw std::bad_alloc(); // Fixme
  }
  m_data = new uchar[ m_row_stride * m_h ];
  clear(*this, bgColor);
}

Bitmap::Bitmap(const IntSize& sz, const Paint& bg)
  : m_row_stride(0),
    m_w(sz.w),
    m_h(sz.h),
    m_data(nullptr)
{
  assert(m_w > 0);
  assert(m_h > 0);
  m_row_stride = faint_cairo_stride(sz);
  if (m_row_stride == -1){
    throw std::bad_alloc(); // Fixme
  }
  m_data = new uchar[ m_row_stride * m_h ];
  clear(*this, bg);
}

Bitmap::Bitmap(const IntSize& sz, int stride)
  : m_row_stride(stride),
    m_w(sz.w),
    m_h(sz.h),
    m_data(nullptr)
{
  assert(sz.w > 0);
  assert(sz.h > 0);
  assert(m_row_stride >= sz.w);
  m_data = new uchar[ m_row_stride * m_h ];
  memset(m_data, 0, to_size_t(m_row_stride * m_h));
}

Bitmap::Bitmap(Bitmap&& source)
  : m_row_stride(source.m_row_stride),
    m_w(source.m_w),
    m_h(source.m_h),
    m_data(source.m_data)
{
  source.m_data = nullptr;
  source.m_w = 0;
  source.m_h = 0;
}

Bitmap::~Bitmap(){
  delete[] m_data;
}

Bitmap& Bitmap::operator=(const Bitmap& other){
  Bitmap temp(copy(other));
  temp.Swap(*this);
  return *this;
}

void Bitmap::Swap(Bitmap& other){
  using std::swap;
  swap(m_row_stride, other.m_row_stride);
  swap(m_w, other.m_w);
  swap(m_h, other.m_h);
  swap(m_data, other.m_data);
}

Bitmap& Bitmap::operator=(Bitmap&& other){
  Bitmap copy(std::move(other));
  copy.Swap(*this);
  return *this;
}

bool bitmap_ok(const Bitmap& bmp){
  return bmp.m_w != 0 && bmp.m_h != 0;
}

void clear(Bitmap& bmp, const Color& c){
  uchar* data = bmp.m_data;
  for (int y = 0; y != bmp.m_h; y++){
    for (int x = 0; x != bmp.m_w; x++){
      int dst = y * bmp.m_row_stride + x * 4;
      data[ dst + iR ] = c.r;
      data[ dst + iG ] = c.g;
      data[ dst + iB ] = c.b;
      data[ dst + iA ] = c.a;
    }
  }
}

void clear(Bitmap& bmp, const ColRGB& c){
  clear(bmp, Color(c, 255));
}

void clear(Bitmap& bmp, const Paint& paint){
  // Fixme: Use dispatch
  if (paint.IsColor()){
    clear(bmp, paint.GetColor());
  }
  else if (paint.IsPattern()){
    clear_pattern(bmp, paint.GetPattern());
  }
  else if (paint.IsGradient()){
    // Fixme: Todo
  }
  else{
    assert(false);
  }
}

Color get_color(const Bitmap& bmp, const IntPoint& pos){
  return get_color_raw(bmp, pos.x, pos.y);
}

Color get_color_raw(const Bitmap& bmp, int x, int y){
  int pos = y * bmp.m_row_stride + x * 4;
  uchar* data = bmp.m_data;
  return Color(data[ pos + iR ], data[pos + iG ], data[ pos + iB ], data[ pos + iA ]);
}

bool inside(const IntRect& r, const Bitmap& bmp){
  if (r.x < 0 || r.y < 0){
    return false;
  }
  if (r.x + r.w > bmp.m_w || r.y + r.h > bmp.m_h){
    return false;
  }
  return true;
}

bool is_blank(const Bitmap& bmp){
  Color color = get_color(bmp, IntPoint(0,0));
  for (int y = 0; y != bmp.m_h; y++){
    for (int x = 0; x != bmp.m_w; x++){
      if (get_color(bmp, IntPoint(x,y)) != color){
        return false;
      }
    }
  }
  return true;
}

bool point_in_bitmap(const Bitmap& bmp, const IntPoint& pos){
  return pos.x >= 0 && pos.y >= 0 && pos.x < bmp.m_w && pos.y < bmp.m_h;
}

void put_pixel(Bitmap& bmp, const IntPoint& pos, const Color& color){
  put_pixel_raw(bmp, pos.x, pos.y, color);
}

void put_pixel_raw(Bitmap& bmp, int x, int y, const Color& color){
  if (x < 0 || y < 0 || x >= bmp.m_w || y >= bmp.m_h) {
    return;
  }
  uchar* data = bmp.GetRaw();
  int pos = y * bmp.m_row_stride + x * 4;
  data[pos + iA] = color.a;
  data[pos + iR] = color.r;
  data[pos + iG] = color.g;
  data[pos + iB] = color.b;
}

} // namespace faint
