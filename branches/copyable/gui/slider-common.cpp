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

#include "bitmap/draw.hh"
#include "geo/geo-func.hh"
#include "geo/intrect.hh"
#include "geo/line.hh"
#include "gui/slider-common.hh"
#include "util/color.hh"

namespace faint{

static void draw_slider_bg_rect(Bitmap& bmp, const IntSize& size){
  IntRect r(IntPoint(0,0), size);
  fill_rect_color(bmp, r, Color(77,109,243));
  draw_rect(bmp, r, {Color(0,0,0), 1, false});
}

void SliderRectangleBackground::Draw(Bitmap& bmp, const IntSize& size){
  draw_slider_bg_rect(bmp, size);
}

SliderBackground* SliderRectangleBackground::Clone() const{
  return new SliderRectangleBackground(*this);
}

void SliderMidPointBackground::Draw(Bitmap& bmp, const IntSize& size){
  draw_slider_bg_rect(bmp, size);
  IntPoint top(size.w / 2, 0);
  IntPoint bottom(top + delta_y(size.h));
  draw_line(bmp, {top, bottom}, {Color(0,0,0), 1, false, LineCap::BUTT});
}

SliderBackground* SliderMidPointBackground::Clone() const{
  return new SliderMidPointBackground(*this);
}

double pos_to_value(const int pos, const int length, const ClosedIntRange& range){
  double pixels_per_value = (length - 2) / double(range.Delta());
  double value = pos / pixels_per_value + range.Lower();
  return value;
}

int value_to_pos(const double value, const int length, const ClosedIntRange& range){
  double pixelsPerValue = (length - 2) / double(range.Delta());
  return rounded(pixelsPerValue * (value - range.Lower()));
}

} // namespace
