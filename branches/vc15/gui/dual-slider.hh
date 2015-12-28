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

#ifndef FAINT_DUAL_SLIDER_HH
#define FAINT_DUAL_SLIDER_HH
#include "wx/event.h"
#include "wx/panel.h"
#include "geo/range.hh"
#include "gui/slider-common.hh"

namespace faint{

class DualSlider : public wxPanel{
  // Interface for sliders with two handles defining an interval
public:
  DualSlider(wxWindow* parent);
  virtual Interval GetSelectedInterval() const = 0;
  virtual void SetSelectedInterval(const Interval&) = 0;
private:
  template<int MINB, int MAXB>
  friend DualSlider* create_dual_slider(wxWindow*, const StaticBoundedInterval<MINB, MAXB>&, const SliderBackground&);
  static DualSlider* Create(wxWindow* parent, const ClosedIntRange&, const Interval&, const SliderBackground&);
};

class DualSliderEvent : public wxCommandEvent{
public:
  DualSliderEvent(const Interval&, bool special);
  wxEvent* Clone() const;
  Interval GetSelectedInterval() const;
  bool Special() const;
private:
  Interval m_interval;
  bool m_special;
};

extern const wxEventTypeTag<DualSliderEvent> EVT_FAINT_DUAL_SLIDER_CHANGE;

// Creates a dual slider control
template<int MINB, int MAXB>
DualSlider* create_dual_slider(wxWindow* parent,
  const StaticBoundedInterval<MINB, MAXB>& values,
  const SliderBackground& bg)
{
  using RangeType = StaticBoundedInterval<MINB, MAXB>;
  return DualSlider::Create(parent,
    make_closed_range(RangeType::min_allowed, RangeType::max_allowed),
    values.GetInterval(),
    bg);
}

} // namespace

#endif
