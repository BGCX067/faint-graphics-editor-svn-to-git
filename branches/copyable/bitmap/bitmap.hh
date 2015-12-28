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

#ifndef FAINT_BITMAP_HH
#define FAINT_BITMAP_HH
#include "geo/geo-fwd.hh"
#include "geo/intsize.hh"
#include "geo/primitive.hh"
#include "util/copyable.hh"
#include "util/paint-fwd.hh"

namespace faint{
extern const uint bpp;
extern const int iR;
extern const int iG;
extern const int iB;
extern const int iA;

class Bitmap {
  // ARGB32 Bitmap
public:
  // Initializes an invalid Bitmap. Must be assigned to before use
  Bitmap();
  Bitmap(const CopySrc<Bitmap>&);
  Bitmap(const IntSize&);
  Bitmap(const IntSize&, const Color&);
  Bitmap(const IntSize&, const Paint&);
  Bitmap(const IntSize&, int stride);
  Bitmap(Bitmap&&);
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

  inline int GetStride() const{
    return m_row_stride;
  }

  void Swap(Bitmap&);

  Bitmap& operator=(const Bitmap&);
  Bitmap& operator=(Bitmap&&);

  int m_row_stride;
  int m_w;
  int m_h;
  uchar* m_data;
private:
  // Fixme, private for now due to:
  // http://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
  Bitmap(const Bitmap&) = delete;
};

bool bitmap_ok(const Bitmap&);
void clear(Bitmap&, const Color&);
void clear(Bitmap&, const ColRGB&);
void clear(Bitmap&, const Paint&);
Color get_color(const Bitmap&, const IntPoint&);
Color get_color_raw(const Bitmap&, int x, int y);
bool inside(const IntRect&, const Bitmap&);
bool is_blank(const Bitmap&);
bool point_in_bitmap(const Bitmap&, const IntPoint&);
void put_pixel(Bitmap&, const IntPoint&, const Color&);
void put_pixel_raw(Bitmap&, int x, int y, const Color&);

class DstBmp{
  // For indicating a target bitmap in Bitmap operations,
  // Should be created using the onto(Bitmap&)-function
public:
  explicit DstBmp(Bitmap& bmp) :
    m_bmp(bmp)
  {}
  uchar* GetRaw(){
    return m_bmp.m_data;
  }
  IntSize GetSize() const{
    return m_bmp.GetSize();
  }
  int GetStride() const{
    return m_bmp.m_row_stride;
  }

  DstBmp& operator=(const DstBmp&) = delete;
private:
  Bitmap& m_bmp;
};

inline DstBmp onto(Bitmap& bmp){
  return DstBmp(bmp);
}

} // namespace faint

#endif
