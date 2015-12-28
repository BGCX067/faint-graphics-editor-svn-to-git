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

#include "wx/dcclient.h"
#include "wx/panel.h"
#include "bitmap/bitmap.hh"
#include "bitmap/draw.hh"
#include "geo/intrect.hh"
#include "geo/line.hh"
#include "gui/mouse-capture.hh"
#include "gui/slider.hh"
#include "util/color.hh"
#include "util/convert-wx.hh"
#include "util/resource-id.hh"

namespace faint{

LineSliderMarker* LineSliderMarker::Clone() const{
  return new LineSliderMarker();
}

void LineSliderMarker::Draw(Bitmap& bmp, SliderDir dir, const IntSize& size, int pos) const{

  auto line = (dir == SliderDir::HORIZONTAL ?
    IntLineSegment({pos, 1}, {pos, size.h - 2}):
    IntLineSegment({0, pos}, {size.w - 2, pos}));

  draw_line(bmp, line, {Color(255,255,255), 1, false, LineCap::BUTT});
}

BorderedSliderMarker* BorderedSliderMarker::Clone() const{
  return new BorderedSliderMarker();
}

void BorderedSliderMarker::Draw(Bitmap& bmp, SliderDir dir, const IntSize& size, int pos) const{
  if (dir == SliderDir::HORIZONTAL ){
    draw_line(bmp, {{pos, 1}, {pos, size.h - 2}},
      {color_white(), 1, false, LineCap::BUTT});
    draw_rect(bmp, {IntPoint(pos - 1, 1), IntPoint(pos + 1, size.h - 2)}, {color_black(), 1, false});
  }
  else if (dir == SliderDir::VERTICAL ){
    draw_line(bmp, {{0, pos}, {size.w - 2, pos}},
      {color_white(), 1, false, LineCap::BUTT});
    draw_rect(bmp, {IntPoint(0, pos - 1), IntPoint(size.w - 2, pos + 1)},
      {color_black(), 1, false});
  }
  else{
    assert(false);
  }
}

Slider::Slider(wxWindow* parent)
  : wxPanel(parent, wxID_ANY)
{}

class SliderImpl : public Slider{
public:
  SliderImpl(wxWindow* parent, const BoundedInt& values, SliderDir dir, const SliderMarker& marker, const SliderBackground& bg )
    : Slider(parent),
      m_background(bg.Clone()),
      m_dir(dir),
      m_marker(marker.Clone()),
      m_mouse(this),
      m_range(values.GetRange()),
      m_value(values.GetValue())
  {
    SetInitialSize(wxSize(20,20)); // Minimum size
    SetCursor(to_wx_cursor(m_dir == SliderDir::HORIZONTAL ? Cursor::VSLIDER : Cursor::HSLIDER ) );

    Bind(wxEVT_LEFT_DOWN, &SliderImpl::OnLeftDown, this);
    Bind(wxEVT_LEFT_UP, &SliderImpl::OnLeftUp, this);
    Bind(wxEVT_MOTION, &SliderImpl::OnMotion, this);
    Bind(wxEVT_PAINT, &SliderImpl::OnPaint, this);
    Bind(wxEVT_SIZE, &SliderImpl::OnSize, this);
    Bind(wxEVT_ERASE_BACKGROUND, &SliderImpl::OnEraseBackground, this);
  }

  ~SliderImpl(){
    delete m_background;
    delete m_marker;
  }

  int GetValue() const override{
    return static_cast<int>(m_value);
  }

  void SetBackground(const SliderBackground& background ) override{
    delete m_background;
    m_background = background.Clone();
    Refresh();
  }

  void SetValue(int value ) override{
    m_value = value;
    Refresh();
  }

private:
  void OnLeftDown(wxMouseEvent& evt ){
    if (m_mouse.HasCapture()){
      return;
    }

    int pos = m_dir == SliderDir::HORIZONTAL ?
      evt.GetPosition().x :
      evt.GetPosition().y;

    int length = m_dir == SliderDir::HORIZONTAL ?
      GetSize().x :
      GetSize().y;

    m_value = m_range.Constrain(pos_to_value(pos, length, m_range));
    m_mouse.Capture();
    Refresh();
    SliderEvent newEvent(GetValue());
    newEvent.SetEventObject(this);
    ProcessEvent(newEvent);
  }

  void OnLeftUp(wxMouseEvent& ){
    if (m_mouse.HasCapture()){
      m_mouse.Release();
      Refresh();
    }
  }

  void OnMotion(wxMouseEvent& evt ){
    if (!m_mouse.HasCapture()){
      return;
    }
    int pos = m_dir == SliderDir::HORIZONTAL ?
      evt.GetPosition().x :
      evt.GetPosition().y;

    int length = m_dir == SliderDir::HORIZONTAL ?
      GetSize().x :
      GetSize().y;

    m_value = m_range.Constrain(pos_to_value(pos, length, m_range ) );
    Refresh();
    SliderEvent newEvent(GetValue());
    newEvent.SetEventObject(this);
    ProcessEvent(newEvent);
  }

  void OnPaint(wxPaintEvent&){
    PrepareBitmap();
    wxPaintDC paintDC(this);
    paintDC.DrawBitmap(m_bitmap, 0, 0);
  }

  void OnSize(wxSizeEvent&){
    Refresh();
  }

  void PrepareBitmap(){
    wxSize sz(GetSize());
    Bitmap bmp(to_faint(sz));
    m_background->Draw(bmp, to_faint(sz));

    int length = m_dir == SliderDir::HORIZONTAL ?
      GetSize().x :
      GetSize().y;

    int pos = value_to_pos(m_value, length, m_range);
    m_marker->Draw(bmp, m_dir, to_faint(sz), pos);
    m_bitmap = to_wx_bmp(bmp);
  }

  void OnEraseBackground(wxEraseEvent& ){
  }

  SliderBackground* m_background;
  wxBitmap m_bitmap;
  SliderDir m_dir;
  SliderMarker* m_marker;
  MouseCapture m_mouse;
  ClosedIntRange m_range;
  double m_value;
};

const wxEventType FAINT_SLIDER_CHANGE = wxNewEventType();

SliderEvent::SliderEvent(int value)
  : wxCommandEvent(FAINT_SLIDER_CHANGE, -1),
    m_value(value)
{}

wxEvent* SliderEvent::Clone() const{
  return new SliderEvent(*this);
}

int SliderEvent::GetValue() const{
  return m_value;
}

const wxEventTypeTag<SliderEvent> EVT_FAINT_SLIDER_CHANGE(FAINT_SLIDER_CHANGE);

Slider* create_slider(wxWindow* parent, const BoundedInt& values, SliderDir dir, const SliderMarker& marker, const SliderBackground& bg ){
  return new SliderImpl(parent, values, dir, marker, bg);
}

Slider* create_slider(wxWindow* parent, const BoundedInt& values, SliderDir dir, const SliderBackground& bg ){
  return new SliderImpl(parent, values, dir, LineSliderMarker(), bg);
}

} // namespace
