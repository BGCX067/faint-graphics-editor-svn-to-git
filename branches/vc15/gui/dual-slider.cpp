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
#include "wx/bitmap.h"
#include "wx/dcclient.h"
#include "app/app.hh" // Fixme: For get_art_container
#include "app/resource-id.hh"
#include "bitmap/bitmap.hh"
#include "bitmap/color.hh"
#include "bitmap/draw.hh"
#include "geo/line.hh"
#include "gui/art-container.hh"
#include "gui/dual-slider.hh"
#include "gui/mouse-capture.hh"
#include "util-wx/bind-event.hh"
#include "util-wx/convert-wx.hh"

namespace faint{
DualSlider::DualSlider(wxWindow* parent)
  : wxPanel(parent)
{}

class DualSliderImpl : public DualSlider{
public:
  DualSliderImpl(wxWindow* parent,
    const ClosedIntRange& range,
    const Interval& startInterval,
    const SliderBackground& bg)
    : DualSlider(parent),
      m_anchor(0),
      m_anchorV1(0.0),
      m_anchorV2(0.0),
      m_background(bg.Clone()),
      m_mouse(this),
      m_range(range),
      m_v1(startInterval.Lower()),
      m_v2(startInterval.Upper()),
      m_which(0)
  {
    assert(m_v1 < m_v2);
    assert(m_range.Has(m_v1));
    assert(m_range.Has(m_v2));

    SetInitialSize(wxSize(20,20)); // Minimum size

    Bind(wxEVT_LEFT_DOWN,
      [this](wxMouseEvent& event){
        MouseDown(event.GetPosition(), false);
      });

    Bind(wxEVT_RIGHT_DOWN,
      [this](wxMouseEvent& event){
        MouseDown(event.GetPosition(), true);
      });

    bind(wxEVT_LEFT_UP, wxEVT_RIGHT_UP, this,
      [this](wxMouseEvent&){
        if (m_mouse.HasCapture()){
          m_mouse.Release();
          Refresh();
        }
      });

    Bind(wxEVT_MOTION,
      [this](wxMouseEvent& evt){
        if (m_mouse.HasCapture()){
          int x = evt.GetPosition().x;
          int w = GetSize().GetWidth();
          if (m_which != 2){
            UpdateFromPos(m_range.Constrain(pos_to_value(x, w, m_range)),
              m_which);
          }
          else{
            UpdateFromAnchor(x);
          }
          Refresh();
          SendSliderChangeEvent();
        }
        else{
          int handle = WhichHandle(evt.GetPosition().x);
          if (handle <= 1){
            SetCursor(get_art_container().Get(Cursor::HORIZONTAL_SLIDER));
          }
          else{
            SetCursor(get_art_container().Get(Cursor::RESIZE_WE));
          }
        }
      });

    Bind(wxEVT_PAINT,
      [this](wxPaintEvent&){
        PrepareBitmap();
        wxPaintDC paintDC(this);
        paintDC.DrawBitmap(m_bitmap, 0, 0);
      });

    Bind(wxEVT_SIZE,
      [this](wxSizeEvent&){
        Refresh();
      });

    Bind(wxEVT_ERASE_BACKGROUND,
      [](wxEraseEvent&){
        // Intentionally left blank
      });
  }

  ~DualSliderImpl(){
    delete m_background;
  }

  Interval GetSelectedInterval() const override{
    return make_interval(floored(m_v1), floored(m_v2));
  }

  void MouseDown(const wxPoint& pos, bool special){
    if (m_mouse.HasCapture()){
      return;
    }
    int x = pos.x;
    m_special = special;
    int handle = WhichHandle(x);
    if (handle == 2){
      m_anchor = x;
      m_anchorV1 = m_v1;
      m_anchorV2 = m_v2;
    }
    m_which = handle;

    if (m_which != 2){
      UpdateFromPos(m_range.Constrain(pos_to_value(x, GetSize().
            GetWidth(), m_range)), m_which);
    }
    else {
      UpdateFromAnchor(x);
    }
    m_mouse.Capture();
    Refresh();
    SendSliderChangeEvent();
  }

  void PrepareBitmap(){
    IntSize sz(to_faint(GetSize()));
    Bitmap bmp(sz);
    m_background->Draw(bmp, sz, SliderDir::HORIZONTAL);
    int x0 = value_to_pos(m_v1, sz.w, m_range);
    int x1 = value_to_pos(m_v2, sz.w, m_range);
    if (x0 > x1){
      std::swap(x0,x1);
    }
    x0 += 1;
    x1 += 1;
    const int xMid = x0 + (x1 - x0) / 2;
    if (x0 != x1){
      Bitmap marked(IntSize(x1 - x0, sz.h - 2), Color(100,100,255,100));
      blend(offsat(marked, {x0,1}), onto(bmp));
      draw_line(bmp, {{x0, 1}, {x0, sz.h-2}},
        {Color(255,255,255), 1, LineStyle::SOLID, LineCap::BUTT});
      draw_line(bmp, {{x1 - 1, 1}, {x1 - 1, sz.h-2}},
        {Color(255,255,255), 1, LineStyle::SOLID, LineCap::BUTT});
      draw_line(bmp, {{xMid, 1}, {xMid, sz.h-2}},
        {Color(255,255,255), 1, LineStyle::LONG_DASH, LineCap::BUTT});
    }
    m_bitmap = to_wx_bmp(bmp);
  }

  void SendSliderChangeEvent(){
    DualSliderEvent newEvent(GetSelectedInterval(), m_special);
    newEvent.SetEventObject(this);
    ProcessEvent(newEvent);
  }

  void SetSelectedInterval(const Interval& interval) override{
    m_v1 = interval.Lower();
    m_v2 = interval.Upper();
    Refresh();
  }

  void UpdateFromAnchor(int x){
    const int dx = m_anchor - x;
    const coord dv = pos_to_value(dx, GetSize().GetWidth(), m_range);
    m_v1 = m_anchorV1 - dv;
    m_v2 = m_anchorV2 - dv;
    double minValue = std::min(m_v1, m_v2);
    double maxValue = std::max(m_v1, m_v2);
    if (minValue < m_range.Lower()){
      m_v1 += m_range.Lower() - minValue;
      m_v2 += m_range.Lower() - minValue;
    }
    if (maxValue > m_range.Upper()){
      m_v1 -= maxValue - m_range.Upper();
      m_v2 -= maxValue - m_range.Upper();
    }
  }

  void UpdateFromPos(double value, int which){
    if (which == 0){
      m_v1 = value;
    }
    else if (which == 1){
      m_v2 = value;
    }
  }

  int WhichHandle(int x){
    IntSize sz(to_faint(GetSize()));
    const int x0 = value_to_pos(m_v1, sz.w, m_range);
    const int x1 = value_to_pos(m_v2, sz.w, m_range);
    int dx0 = std::abs(x0 - x);
    int dx1 = std::abs(x1 - x);
    int xMid = x0 + (x1 - x0) / 2;
    int dxA = std::abs(xMid - x);

    if (dx0 <= dx1 && (dx0 <= dxA || dxA > 5)){
      return 0;
    }
    else if (dx1 < dx0 && (dx1 < dxA || dxA > 5)){
      return 1;
    }
    else {
      return 2;
    }
  }

  int m_anchor; // For moving the entire area
  double m_anchorV1;
  double m_anchorV2;
  SliderBackground* m_background;
  wxBitmap m_bitmap;
  MouseCapture m_mouse;
  ClosedIntRange m_range;
  double m_v1;
  double m_v2;
  int m_which;
  bool m_special = false;
};

const wxEventType FAINT_DUAL_SLIDER_CHANGE = wxNewEventType();

DualSliderEvent::DualSliderEvent(const Interval& interval, bool special)
  : wxCommandEvent(FAINT_DUAL_SLIDER_CHANGE, -1),
    m_interval(interval),
    m_special(special)
{}

wxEvent* DualSliderEvent::Clone() const{
  return new DualSliderEvent(*this);
}

Interval DualSliderEvent::GetSelectedInterval() const{
  return m_interval;
}

bool DualSliderEvent::Special() const{
  return m_special;
}

const wxEventTypeTag<DualSliderEvent> EVT_FAINT_DUAL_SLIDER_CHANGE(FAINT_DUAL_SLIDER_CHANGE);

DualSlider* DualSlider::Create(wxWindow* parent,
  const ClosedIntRange& range,
  const Interval& startInterval,
  const SliderBackground& bg)
{
  return new DualSliderImpl(parent, range, startInterval, bg);
}

} // namespace
