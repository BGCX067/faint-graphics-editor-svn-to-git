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

#include <random>
#include "gui/dual-slider.hh"
#include "gui/slider.hh"
#include "gui/slider-alpha-background.hh"
#include "gui/slider-histogram-background.hh"

namespace faint{ class StatusInterface; }
namespace faint{ class DialogContext; }

static std::vector<int> histogram_test_values(){
  std::random_device device;
  std::mt19937 generator(device());
  std::normal_distribution<> distribution(128.0, 50.0);

  std::vector<int> values(256, 0);
  for (int n = 0; n != 10000; n++){
    int i = std::min(255, std::max(0, static_cast<int>(distribution(generator))));
    values[i] += 1;
  }
  return values;
}

void gui_test_slider(wxWindow* p, faint::StatusInterface&,
  faint::DialogContext&)
{
  using namespace faint;
  auto s1 = create_slider(p, BoundedInt(min_t(0), 45, max_t(120)),
    SliderDir::HORIZONTAL,
    LineSliderMarker(),
    SliderRectangleBackground());
  s1->SetSize(wxSize(200,20));
  s1->SetPosition(wxPoint(10,10));

  auto s2 = create_slider(p, BoundedInt(min_t(0), 45, max_t(120)),
    SliderDir::HORIZONTAL,
    BorderedSliderMarker(),
    SliderRectangleBackground());
  s2->SetSize(wxSize(200,20));
  s2->SetPosition(wxPoint(10,40));

  auto s3 = create_slider(p, BoundedInt(min_t(0), 45, max_t(120)),
    SliderDir::HORIZONTAL,
    BorderedSliderMarker(),
    AlphaBackground(ColRGB(255,0,0)));
  s3->SetSize(wxSize(200,20));
  s3->SetPosition(wxPoint(10,70));

  auto s4 = create_slider(p, BoundedInt(min_t(0), 45, max_t(255)),
    SliderDir::HORIZONTAL,
    BorderedSliderMarker(),
    SliderHistogramBackground(histogram_test_values(),
      ColRGB(100,100,200),
      ColRGB(0,0,0)));
  s4->SetSize(wxSize(200,20));
  s4->SetPosition(wxPoint(10,100));

  auto s5 = create_dual_slider(p,
    StaticBoundedInterval<0, 100>(Interval(0, 80)),
    AlphaBackground(ColRGB(0,128,0)));
  s5->SetSize(wxSize(200,20));
  s5->SetPosition(wxPoint(10,130));

  auto s6 = create_slider(p, BoundedInt(min_t(0), 45, max_t(120)),
    SliderDir::VERTICAL,
    BorderedSliderMarker(),
    SliderRectangleBackground());
  s6->SetSize(wxSize(20,230));
  s6->SetPosition(wxPoint(10,160));

  auto s7 = create_slider(p, BoundedInt(min_t(0), 45, max_t(120)),
    SliderDir::VERTICAL,
    BorderedSliderMarker(),
    AlphaBackground(ColRGB(0,0,255)));
  s7->SetSize(wxSize(20,230));
  s7->SetPosition(wxPoint(40,160));
}
