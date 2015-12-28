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

#include "wx/wx.h"
#include "bitmap/bitmap.hh"
#include "gui/dual-slider.hh"
#include "util/color.hh"
#include "util/convertwx.hh"

static double x_to_value( const int x, const wxSize& sz, const ClosedIntRange& range ){
  double pixels_per_value = ( sz.GetWidth() - 2 ) / double(range.Delta());
  double value = x / pixels_per_value;
  return value;
}

static int value_to_x( const double value, const wxSize& sz, const ClosedIntRange& range ){
  double pixels_per_value = ( sz.GetWidth() - 2 ) / double(range.Delta());
  return static_cast<int>(value * pixels_per_value); // Fixme: round?
}

DualSlider::DualSlider(wxWindow* parent, const ClosedIntRange& range, const Interval& startInterval , const SliderBackground& bg )
  : wxPanel(parent, wxID_ANY),
    m_anchor(0),
    m_anchorV1(0.0),
    m_anchorV2(0.0),
    m_background(bg.Clone()),
    m_range(range),
    m_v1(startInterval.Lower()),
    m_v2(startInterval.Upper()),
    m_which(0)
{
  assert(m_v1 < m_v2);
  assert(m_range.Has(m_v1));
  assert(m_range.Has(m_v2));

  SetInitialSize(wxSize(20,20)); // Minimum size
}

DualSlider::~DualSlider(){
  delete m_background;
}

Interval DualSlider::GetSelectedInterval() const{
  return make_interval(floored(m_v1), floored(m_v2));
}

void DualSlider::OnCaptureLost( wxMouseCaptureLostEvent& ){
  // Required for MSW
}

void DualSlider::OnEraseBackground( wxEraseEvent& ){
}

void DualSlider::OnLeftDown( wxMouseEvent& evt ){
  int x = evt.GetPosition().x;
  int handle = WhichHandle( x );
  if ( handle == 2 ){
    m_anchor = x;
    m_anchorV1 = m_v1;
    m_anchorV2 = m_v2;
  }
  m_which = handle;

  if ( m_which != 2 ){
    UpdateFromPos( m_range.Constrain(x_to_value( x, GetSize(), m_range )), m_which );
  }
  else {
    UpdateFromAnchor( x );
  }
  CaptureMouse();
  Refresh();
}

void DualSlider::OnLeftUp( wxMouseEvent& ){
  if ( HasCapture() ){
    ReleaseMouse();
    Refresh();
  }
}
void DualSlider::OnMotion( wxMouseEvent& evt ){
  if ( HasCapture() ){
    int x = evt.GetPosition().x;
    wxSize sz( GetSize() );
    if ( m_which != 2 ){
      UpdateFromPos( m_range.Constrain( x_to_value( x, sz, m_range ) ), m_which ); // Fixme - rename etc.
    }
    else {
      UpdateFromAnchor(x);
    }
    Refresh();
    SendSliderChangeEvent();
  }
  else {
    int handle = WhichHandle(evt.GetPosition().x);
    if ( handle <= 1 ){
      SetCursor( to_wx_cursor(Cursor::VSLIDER) );
    }
    else {
      SetCursor( to_wx_cursor(Cursor::RESIZE_WE) );
    }
  }
}

void DualSlider::OnPaint( wxPaintEvent& ){
  PrepareBitmap();
  wxPaintDC paintDC(this);
  paintDC.DrawBitmap(m_bitmap, 0, 0);
}

void DualSlider::OnSize( wxSizeEvent& ){
  Refresh();
}

void DualSlider::PrepareBitmap(){
  wxSize sz(GetSize());
  faint::Bitmap bmp(to_faint(sz));
  m_background->Draw(bmp, to_faint(sz));
  int x0 = value_to_x(m_v1, sz, m_range);
  int x1 = value_to_x(m_v2, sz, m_range);
  if ( x0 > x1 ){
    std::swap(x0,x1);
  }
  x0 += 1;
  x1 += 1;
  const int xMid = x0 + ( x1 - x0 ) / 2;
  const int h = sz.GetHeight();
  if ( x0 != x1 ){
    faint::Bitmap marked(IntSize(x1 - x0, h - 2), faint::Color(100,100,255,100));
    faint::blend(marked, onto(bmp), IntPoint(x0,1));
    faint::draw_line_color(bmp, IntPoint(x0, 1), IntPoint(x0, h-2), faint::Color(255,255,255), 1, false, LineCap::BUTT);
    faint::draw_line_color(bmp, IntPoint(x1 - 1, 1), IntPoint(x1 - 1, h-2), faint::Color(255,255,255), 1, false, LineCap::BUTT);
    faint::draw_line_color(bmp, IntPoint(xMid, 1), IntPoint(xMid, h-2), faint::Color(255,255,255), 1, true, LineCap::BUTT);
  }
  m_bitmap = to_wx_bmp(bmp);
}

void DualSlider::SendSliderChangeEvent(){
  DualSliderEvent newEvent(FAINT_DUAL_SLIDER_CHANGE, GetSelectedInterval());
  newEvent.SetEventObject(this);
  ProcessEvent(newEvent);
}

void DualSlider::UpdateFromAnchor( int x ){
  int dx = m_anchor - x;
  m_v1 = m_anchorV1 - x_to_value( dx, GetSize(), m_range);
  m_v2 = m_anchorV2 - x_to_value( dx, GetSize(), m_range);
  double minValue = std::min(m_v1, m_v2);
  double maxValue = std::max(m_v1, m_v2);
  if ( minValue < m_range.Lower() ){
    m_v1 += m_range.Lower() - minValue;
    m_v2 += m_range.Lower() - minValue;
  }
  if ( maxValue > m_range.Upper() ){
    m_v1 -= maxValue - m_range.Upper();
    m_v2 -= maxValue - m_range.Upper();
  }
}

void DualSlider::UpdateFromPos( double value, int which ){
  if ( which == 0 ){
    m_v1 = value;
  }
  else if ( which == 1 ){
    m_v2 = value;
  }
}

int DualSlider::WhichHandle( int x ){
  wxSize sz(GetSize());
  const int x0 = value_to_x( m_v1, sz, m_range );
  const int x1 = value_to_x( m_v2, sz, m_range );
  int dx0 = abs(x0 - x);
  int dx1 = abs(x1 - x);
  int xMid = x0 + ( x1 - x0 ) / 2;
  int dxA = abs(xMid - x);

  if ( dx0 <= dx1 && (dx0 <= dxA || dxA > 5) ){
    return 0;
  }
  else if ( dx1 < dx0 && ( dx1 < dxA || dxA > 5)){
    return 1;
  }
  else {
    return 2;
  }
}

const wxEventType FAINT_DUAL_SLIDER_CHANGE = wxNewEventType();

DualSliderEvent::DualSliderEvent( wxEventType type, const Interval& interval )
  : wxCommandEvent(type, -1),
    m_interval(interval)
{}

wxEvent* DualSliderEvent::Clone() const{
  return new DualSliderEvent(*this);
}

Interval DualSliderEvent::GetSelectedInterval() const{
  return m_interval;
}

BEGIN_EVENT_TABLE(DualSlider, wxPanel)
EVT_LEFT_DOWN(DualSlider::OnLeftDown)
EVT_LEFT_UP(DualSlider::OnLeftUp)
EVT_MOTION(DualSlider::OnMotion)
EVT_MOUSE_CAPTURE_LOST(DualSlider::OnCaptureLost)
EVT_PAINT(DualSlider::OnPaint)
EVT_SIZE(DualSlider::OnSize)
EVT_ERASE_BACKGROUND( DualSlider::OnEraseBackground )
END_EVENT_TABLE()
