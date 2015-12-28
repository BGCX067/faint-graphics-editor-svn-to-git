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
#include "gui/cursors.hh"
#include "gui/slider.hh"
#include "util/color.hh"
#include "util/convertwx.hh"

void SliderRectangleBackground::Draw( faint::Bitmap& bmp, const IntSize& size ){
  IntRect r(IntPoint(0,0), size);
  faint::fill_rect_color( bmp, r, faint::Color(77,109,243) );
  faint::draw_rect_color( bmp, r, faint::Color(0,0,0), 1, false );
}

SliderBackground* SliderRectangleBackground::Clone() const{
  return new SliderRectangleBackground(*this);
}

static double pos_to_value( const int pos, const int length, const ClosedIntRange& range ){
  double pixels_per_value = ( length - 2 ) / double(range.Delta());
  double value = pos / pixels_per_value;
  return value;
}

static int value_to_pos( const double value, const int length, const ClosedIntRange& range ){
  double pixels_per_value = ( length - 2 ) / double(range.Delta());
  return static_cast<int>(value * pixels_per_value); // Fixme: round?
}

Slider::Slider(wxWindow* parent, const ClosedIntRange& range, int value, SliderDir dir, const SliderBackground& bg )
  : wxPanel(parent, wxID_ANY),
    m_background(bg.Clone()),
    m_dir(dir),
    m_range(range),
    m_value(value)
{
  assert(m_range.Has(value));
  SetInitialSize(wxSize(20,20)); // Minimum size
  SetCursor(to_wx_cursor(
    m_dir == SliderDir::HORIZONTAL ?
    Cursor::VSLIDER :
    Cursor::HSLIDER ) );
}

Slider::~Slider(){
  delete m_background;
}

int Slider::GetValue() const{
  return static_cast<int>(m_value);
}

void Slider::SetBackground( const SliderBackground& background ){
  delete m_background;
  m_background = background.Clone();
  Refresh();
}

void Slider::SetValue( int value ){
  m_value = value;
  Refresh();
  //SliderEvent newEvent(FAINT_SLIDER_CHANGE, GetValue());
  //newEvent.SetEventObject(this);
  //ProcessEvent(newEvent);
}

void Slider::OnCaptureLost( wxMouseCaptureLostEvent& ){
  // Required for MSW
}

void Slider::OnLeftDown( wxMouseEvent& evt ){
  int pos = m_dir == SliderDir::HORIZONTAL ? evt.GetPosition().x : evt.GetPosition().y;
  int length = m_dir == SliderDir::HORIZONTAL ? GetSize().x : GetSize().y;
  m_value = m_range.Constrain(pos_to_value( pos, length, m_range ));
  CaptureMouse();
  Refresh();
  SliderEvent newEvent(FAINT_SLIDER_CHANGE, GetValue());
  newEvent.SetEventObject(this);
  ProcessEvent(newEvent);
}

void Slider::OnLeftUp( wxMouseEvent& ){
  if ( HasCapture() ){
    ReleaseMouse();
    Refresh();
  }
}
void Slider::OnMotion( wxMouseEvent& evt ){
  if ( !HasCapture() ){
    return;
  }
  int pos = m_dir == SliderDir::HORIZONTAL ? evt.GetPosition().x : evt.GetPosition().y;
  int length = m_dir == SliderDir::HORIZONTAL ? GetSize().x : GetSize().y;
  m_value = m_range.Constrain( pos_to_value( pos, length, m_range ) );
  Refresh();
  SliderEvent newEvent(FAINT_SLIDER_CHANGE, GetValue());
  newEvent.SetEventObject(this);
  ProcessEvent(newEvent);
}

void Slider::OnPaint( wxPaintEvent& ){
  PrepareBitmap();
  wxPaintDC paintDC(this);
  paintDC.DrawBitmap(m_bitmap, 0, 0);
}

void Slider::OnSize( wxSizeEvent& ){
  Refresh();
}

void Slider::PrepareBitmap(){
  wxSize sz(GetSize());
  faint::Bitmap bmp(to_faint(sz));
  m_background->Draw(bmp, to_faint(sz));
  int length = m_dir == SliderDir::HORIZONTAL ? GetSize().x : GetSize().y;
  int pos = value_to_pos(m_value, length, m_range);

  if ( m_dir == SliderDir::HORIZONTAL ){
    const int h = sz.GetHeight();
    faint::draw_line_color(bmp, IntPoint(pos, 1), IntPoint(pos, h - 2), faint::Color(255,255,255), 1, false, LineCap::BUTT);
  }
  else {
    const int w = sz.GetWidth();
    faint::draw_line_color(bmp, IntPoint(0, pos), IntPoint(w - 2, pos), faint::Color(255,255,255), 1, false, LineCap::BUTT);
  }
  m_bitmap = to_wx_bmp(bmp);
}

void Slider::OnEraseBackground( wxEraseEvent& ){
}

const wxEventType FAINT_SLIDER_CHANGE = wxNewEventType();

SliderEvent::SliderEvent( wxEventType type, int value )
  : wxCommandEvent(type, -1),
    m_value(value)
{}

wxEvent* SliderEvent::Clone() const{
  return new SliderEvent(*this);
}

int SliderEvent::GetValue() const{
  return m_value;
}

BEGIN_EVENT_TABLE(Slider, wxPanel)
EVT_LEFT_DOWN(Slider::OnLeftDown)
EVT_LEFT_UP(Slider::OnLeftUp)
EVT_MOTION(Slider::OnMotion)
EVT_MOUSE_CAPTURE_LOST(Slider::OnCaptureLost)
EVT_PAINT(Slider::OnPaint)
EVT_SIZE(Slider::OnSize)
EVT_ERASE_BACKGROUND( Slider::OnEraseBackground )
END_EVENT_TABLE()
