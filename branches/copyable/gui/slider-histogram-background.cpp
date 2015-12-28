// -*- coding: us-ascii-unix -*-
// Copyright 2013 Lukas Kemmer
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
#include "bitmap/draw.hh"
#include "bitmap/histogram.hh"
#include "geo/intrect.hh"
#include "geo/primitive.hh"
#include "gui/slider-histogram-background.hh"

namespace faint{

SliderHistogramBackground::SliderHistogramBackground(const std::vector<int>& values, const ColRGB& bg, const ColRGB& fg)
  : m_bitmap(),
    m_values(values),
    m_bg(bg),
    m_fg(fg)
{}

SliderHistogramBackground::SliderHistogramBackground(const std::vector<int>& values, const ColRGB& fg)
  : m_bitmap(),
    m_values(values),
    m_bg(ColRGB(200,200,200)),
    m_fg(fg)
{}

void SliderHistogramBackground::Draw(Bitmap& bmp, const IntSize& size) {
  if (!bitmap_ok(m_bitmap) || m_bitmap.obj.GetSize() != size){
    InitializeBitmap(size);
  }
  blit(m_bitmap, onto(bmp), IntPoint(0,0));
}

SliderBackground* SliderHistogramBackground::Clone() const {
  return new SliderHistogramBackground(*this);
}

void SliderHistogramBackground::InitializeBitmap(const IntSize& size){
  const int numValues = resigned(m_values.size());
  const bin_t bins(std::min(numValues, size.w));
  Histogram histogram(ClosedIntRange(min_t(0), max_t(numValues)), bins);
  for (int i = 0; i != numValues; i++){
    histogram.Insert(i, count_t(m_values[to_size_t(i)]));
  }
  int max = histogram.Max();

  double binWidth = std::max(size.w / double(histogram.NumBins()), 1.0); // Pixels per bin
  double scale_h = size.h / double(max);
  m_bitmap = Bitmap(size, Color(m_bg,255));
  for (int bin = 0; bin < histogram.NumBins(); bin++){
    int x = static_cast<int>(bin * binWidth);
    int y = static_cast<int>(size.h - histogram.Count(bin_t(bin)) * scale_h);
    fill_rect_color(m_bitmap, IntRect(IntPoint(x,y), IntSize(int(binWidth) + 1, size.h)), Color(m_fg,255));
  }
  draw_rect(m_bitmap, IntRect(IntPoint(0,0), size),
    {Color(0, 0, 0), 1, false});
}

}
