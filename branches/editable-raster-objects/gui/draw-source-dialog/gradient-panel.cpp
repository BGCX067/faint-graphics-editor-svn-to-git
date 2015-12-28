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
#include <cmath>
#include "wx/wx.h"
#include "bitmap/bitmap.hh"
#include "geo/intsize.hh"
#include "gui/bitmaplist.hh"
#include "gui/draw-source-dialog.hh"
#include "gui/draw-source-dialog/gradient-panel.hh"
#include "gui/draw-source-dialog/placement.hh"
#include "gui/layout.hh"
#include "util/colorutil.hh"
#include "util/convertwx.hh"
#include "util/util.hh"

static IntPoint window_center( const wxWindow* win ){
  wxSize size(win->GetSize());
  return IntPoint(size.GetWidth() / 2, size.GetHeight() / 2);
}

static IntPoint relative_to_center( const IntPoint& p, const wxWindow* win ){
  return p - window_center(win);
}

static int handle_hit_test( const IntPoint& pos, const faint::color_stops_t& stops, const IntSize& sz ){
  int numStops = resigned(stops.size());
  for ( int i = 0; i != numStops; i++ ){
    const faint::ColorStop& s(stops[i]);
    int center = 5 + truncated((sz.w - 10) * s.GetOffset()); // Fixme: Scattered info, move into ColorStopSlider class
    if ( abs(center - pos.x) < 5 ){
      return i;
    }
  }
  return -1;
}

static faint::Bitmap with_angle_indicator( const faint::Bitmap& src, faint::radian angle ){
  IntSize sz(src.GetSize());
  int dx = static_cast<int>(cos(-angle) * sz.w);
  int dy = static_cast<int>(sin(-angle) * sz.w);
  int cx = sz.w / 2;
  int cy = sz.h / 2;
  faint::Bitmap dst(src);
  faint::draw_line_color( dst,
    IntPoint(cx,cy),
    IntPoint(cx+dx,cy+dy), faint::Color(255,255,255), 1, false, LineCap::BUTT);
  return dst;
}

static faint::LinearGradient default_linear_gradient(){
  using faint::Color;
  using faint::ColorStop;
  faint::color_stops_t stops;
  stops.push_back(ColorStop(Color(255,0,0), 0.0));
  stops.push_back(ColorStop(Color(0,0,0), 1.0));
  return faint::LinearGradient(0.0, stops);
}

template<typename T>
static faint::Color get_stop_color(int index, const T& g ){
  return g.GetStop(index).GetColor();
}

static faint::Bitmap gradient_handles_bitmap( const faint::color_stops_t& stops, const IntSize& sz ){
  faint::Bitmap bmp(sz, faint::color_white());
  int w = 5;
  for ( size_t i = 0; i != stops.size(); i++ ){
    const faint::ColorStop& s(stops[i]);
    int cx = 5 + floored((sz.w - 11) * s.GetOffset());
    std::vector<IntPoint> pts;
    pts.push_back(IntPoint(cx, 0));
    pts.push_back(IntPoint(cx + w, sz.h - 1));
    pts.push_back(IntPoint(cx - w, sz.h - 1));
    faint::Pattern pattern(color_bitmap(s.GetColor(), IntSize(6,6)));
    fill_polygon(bmp, pts, faint::DrawSource(pattern));
    draw_polygon(bmp, pts, faint::DrawSource(faint::color_black()), 1, false);
  }
  return bmp;
}

DEFINE_EVENT_TYPE(EVT_GRADIENT_SLIDER_CHANGE)

template<typename T>
class ColorStopSlider : public wxPanel {
public:
  ColorStopSlider(wxWindow* parent, wxSize size, T& gradient )
    : wxPanel(parent, wxID_ANY),
      m_gradient(gradient),
      m_handle(-1)
  {
    SetInitialSize(size);
    SetBackgroundStyle( wxBG_STYLE_PAINT );
    Connect(wxID_ANY, wxEVT_MOUSE_CAPTURE_LOST, wxMouseCaptureLostEventHandler(ColorStopSlider<T>::OnCaptureLost));
    Connect(wxID_ANY, wxEVT_LEFT_DOWN, wxMouseEventHandler(ColorStopSlider<T>::OnLeftDown));
    Connect(wxID_ANY, wxEVT_LEFT_UP, wxMouseEventHandler(ColorStopSlider<T>::OnLeftUp));
    Connect(wxID_ANY, wxEVT_MOTION, wxMouseEventHandler(ColorStopSlider<T>::OnMotion));
    Connect(wxID_ANY, wxEVT_PAINT, wxPaintEventHandler(ColorStopSlider<T>::OnPaint));
    Connect(wxID_ANY, wxEVT_RIGHT_DOWN, wxMouseEventHandler(ColorStopSlider<T>::OnRightDown));
    UpdateGradient();
  }

  void UpdateGradient(){
    m_bmp = to_wx_bmp(gradient_handles_bitmap( m_gradient.GetStops(), to_faint(GetSize())));
  }

private:
  void OnCaptureLost( wxMouseCaptureLostEvent& ){
    // Empty, but required on MSW
  }

  void OnLeftDown( wxMouseEvent& event ){
    m_handle = handle_hit_test(to_faint(event.GetPosition()), m_gradient.GetStops(), to_faint(GetSize()));
    if ( m_handle == -1 ){
      return;
    }
    if ( event.ControlDown() ){
      m_gradient.Add(m_gradient.GetStop(m_handle));
      m_handle = m_gradient.GetNumStops() - 1;
    }
    CaptureMouse();
    SetHandleFromPos(to_faint(event.GetPosition()));
  }

  void OnLeftUp( wxMouseEvent& ){
    if ( HasCapture() ){
      ReleaseMouse();
    }
  }

  void OnMotion( wxMouseEvent& event ){
    if ( HasCapture() ){
      SetHandleFromPos(to_faint(event.GetPosition()));
    }
    else {
      m_handle = handle_hit_test(to_faint(event.GetPosition()), m_gradient.GetStops(), to_faint(GetSize()));
      if ( m_handle == - 1 ){
        SetCursor(wxCURSOR_ARROW);
      }
      else{
        SetCursor(wxCURSOR_SIZEWE);
      }
    }
  }

  void OnPaint( wxPaintEvent& ){
    wxPaintDC dc(this);
    dc.DrawBitmap(m_bmp, 0, 0);
  }

  void OnRightDown( wxMouseEvent& event ){
    m_handle = handle_hit_test(to_faint(event.GetPosition()), m_gradient.GetStops(), to_faint(GetSize()));
    if ( m_handle == -1 ){
      return;
    }
    if ( event.ControlDown() ){
      if ( m_gradient.GetNumStops() > 1 ){
        m_gradient.Remove(m_handle);
        UpdateGradient();
        SendChangeEvent();
        Refresh();
      }
    }
    else{
      faint::ColorStop stop(m_gradient.GetStop(m_handle));
      Optional<faint::Color> c = show_color_only_dialog(this, "Select Stop Color", stop.GetColor());
      if ( c.IsSet() ){
        m_gradient.SetStop(m_handle, faint::ColorStop(c.Get(), stop.GetOffset()));
        UpdateGradient();
        SendChangeEvent();
        Refresh();
      }
    }
  }

  void SendChangeEvent(){
    wxCommandEvent changeEvent(EVT_GRADIENT_SLIDER_CHANGE, GetId());
    GetEventHandler()->ProcessEvent(changeEvent);
  }

  void SetHandleFromPos( const IntPoint& mousePos ){
    faint::coord x = constrained(Min(0.0), mousePos.x / floated(GetSize().GetWidth()), Max(1.0));
    faint::ColorStop stop( get_stop_color(m_handle, m_gradient), x );
    m_gradient.SetStop(m_handle, stop );
    UpdateGradient();
    SendChangeEvent();
    Refresh();
  }

  T& m_gradient;
  wxBitmap m_bmp;
  int m_handle;
};

typedef ColorStopSlider<faint::LinearGradient> LinearSlider;
typedef ColorStopSlider<faint::RadialGradient> RadialSlider;
static const int g_sliderHeight = 20;

class LinearGradientDisplay : public wxPanel {
public:
  LinearGradientDisplay( wxWindow* parent, const wxSize& size ):
    wxPanel(parent, wxID_ANY)
  {
    SetBackgroundStyle( wxBG_STYLE_PAINT ); // Prevent flicker on full refresh
    SetInitialSize(size);
    m_slider = new LinearSlider(this, wxSize(size.GetWidth(), g_sliderHeight), m_gradient);
    m_slider->SetPosition(wxPoint(0, size.GetHeight() - g_sliderHeight));
  }

  faint::LinearGradient GetGradient() const{
    return m_gradient;
  }

  void SetGradient( const faint::LinearGradient& g ){
    m_gradient = g;
    UpdateBitmap();
    m_slider->UpdateGradient();
    Refresh();
  }

  void SetStops( const faint::color_stops_t& stops ){
    m_gradient.SetStops(stops);
    m_slider->UpdateGradient();
    UpdateBitmap();
    Refresh();
  }

private:
  void OnCaptureLost( wxMouseCaptureLostEvent& ){
    // Empty, but required on MSW.
  }

  void OnLeftDown( wxMouseEvent& event ){
    int max_x = m_bmp.GetWidth();
    int max_y = m_bmp.GetHeight();
    wxPoint pos(event.GetPosition());
    if ( pos.x > max_x || pos.y > max_y ){
      return;
    }
    CaptureMouse();
    SetAngleFromPos(to_faint(pos));
  }

  void OnLeftUp( wxMouseEvent& ){
    if ( HasCapture() ){
      ReleaseMouse();
    }
  }

  void OnMotion( wxMouseEvent& event ){
    if ( HasCapture() ){
      IntPoint pos(to_faint(event.GetPosition()));
      if ( event.ShiftDown() ){
        pos = floored(adjust_to_45( floated(window_center(this)), floated(pos) ));
      }
      SetAngleFromPos( pos );
    }
  }

  void OnPaint( wxPaintEvent& ){
    wxPaintDC dc(this);
    dc.DrawBitmap(m_bmp, 0, 0);
  }

  void OnSliderChange( wxCommandEvent& ){
    UpdateBitmap();
    Refresh();
  }

  void SetAngleFromPos( const IntPoint& mousePos ){
    IntPoint pos = relative_to_center(mousePos, this); // Fixme: Wrong
    if ( pos.x == 0 && pos.y == 0 ){
      return;
    }
    double radians = atan2( static_cast<double>(pos.y), static_cast<double>(pos.x) );

    m_gradient.SetAngle(-radians);
    UpdateBitmap();
    Refresh();
  }

  void UpdateBitmap(){
    IntSize sz(to_faint(GetSize()));
    faint::Bitmap bg(sz, faint::color_white());
    m_gradientBmp = faint::gradient_bitmap(faint::Gradient(unrotated(m_gradient)), sz - IntSize(10,g_sliderHeight)); // Fixme: Literals
    blit( with_border(with_angle_indicator(m_gradientBmp, m_gradient.GetAngle())), onto(bg), IntPoint(5,0));
    m_bmp = to_wx_bmp(bg);
  }

  wxBitmap m_bmp;
  faint::LinearGradient m_gradient;
  faint::Bitmap m_gradientBmp;
  LinearSlider* m_slider;
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(LinearGradientDisplay, wxPanel)
EVT_MOUSE_CAPTURE_LOST(LinearGradientDisplay::OnCaptureLost)
EVT_PAINT(LinearGradientDisplay::OnPaint)
EVT_LEFT_DOWN(LinearGradientDisplay::OnLeftDown)
EVT_LEFT_UP(LinearGradientDisplay::OnLeftUp)
EVT_MOTION(LinearGradientDisplay::OnMotion)
EVT_COMMAND(-1, EVT_GRADIENT_SLIDER_CHANGE, LinearGradientDisplay::OnSliderChange)
END_EVENT_TABLE()

class RadialGradientDisplay : public wxPanel {
public:
  RadialGradientDisplay( wxWindow* parent, const wxSize& size )
    : wxPanel(parent, wxID_ANY)
  {
    SetBackgroundStyle( wxBG_STYLE_PAINT ); // Prevent flicker on full refresh
    SetInitialSize(size);
    m_slider = new RadialSlider(this, wxSize(size.GetWidth(), g_sliderHeight), m_gradient);
    m_slider->SetPosition(wxPoint(0, size.GetHeight() - g_sliderHeight)); // Fixme
  }

  faint::RadialGradient GetGradient() const{
    return m_gradient;
  }

  void SetGradient( const faint::RadialGradient& g ){
    m_gradient = g;
    UpdateBitmap();
    m_slider->UpdateGradient();
    Refresh();
  }

  void SetStops( const faint::color_stops_t& stops ){
    m_gradient.SetStops(stops);
    UpdateBitmap();
    m_slider->UpdateGradient();
    Refresh();
  }
private:
  void OnPaint( wxPaintEvent& event ){
    wxPaintDC dc(this);
    dc.DrawBitmap(m_bmp, 0, 0);
    event.Skip();
  }

  void OnRightDown( wxMouseEvent& ){
    // Fixme
  }

  void OnSliderChange( wxCommandEvent& ){
    UpdateBitmap();
    Refresh();
  }

  void UpdateBitmap(){
    IntSize sz(to_faint(GetSize()));
    faint::Bitmap bg(sz - IntSize(0, g_sliderHeight), faint::color_white());

    IntSize gSz(sz - IntSize(10, g_sliderHeight));
    m_gradientBmp = faint::sub_bitmap(faint::gradient_bitmap(faint::Gradient(m_gradient), IntSize(gSz.w * 2, gSz.h)),
      IntRect(IntPoint(gSz.w,0), gSz));
    blit( with_border(m_gradientBmp), onto(bg), IntPoint(5,0) );
    m_bmp = to_wx_bmp(bg);
  }

  wxBitmap m_bmp;
  faint::RadialGradient m_gradient;
  faint::Bitmap m_gradientBmp;
  RadialSlider* m_slider;
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(RadialGradientDisplay, wxPanel)
EVT_COMMAND(-1, EVT_GRADIENT_SLIDER_CHANGE, RadialGradientDisplay::OnSliderChange)
EVT_PAINT(RadialGradientDisplay::OnPaint)
END_EVENT_TABLE()

static std::vector<faint::ColorStop> icon_color_stops(){
  using faint::Color;
  using faint::ColorStop;
  std::vector<ColorStop> stops;
  stops.push_back(ColorStop(Color(0,0,0), 0.0));
  stops.push_back(ColorStop(Color(255,255,255), 1.0));
  return stops;
}

static wxBitmap linear_icon( const IntSize& size ){
  faint::LinearGradient g(0.0, icon_color_stops());
  faint::Bitmap bmp(faint::gradient_bitmap(faint::Gradient(g), size));
  return to_wx_bmp(bmp);
}

static wxBitmap radial_icon( const IntSize& size ){
  Radii radii(floated(size.w), floated(size.h));
  faint::RadialGradient g(Point(0,0), radii, icon_color_stops());
  faint::Bitmap bmp(faint::gradient_bitmap(faint::Gradient(g), size));
  return to_wx_bmp(bmp);
}

class ColorPanel_Gradient_Impl : public wxPanel{
public:
  ColorPanel_Gradient_Impl( wxWindow* parent, StatusInterface& statusInfo )
    : wxPanel( parent, wxID_ANY )
  {
    using faint::panel_padding;
    const wxSize displaySize(420,100);
    const wxPoint displayTopLeft(panel_padding, panel_padding);

    m_linearDisplay = new LinearGradientDisplay(this, displaySize);
    m_linearDisplay->SetPosition(displayTopLeft);
    m_linearDisplay->Hide();

    m_radialDisplay = new RadialGradientDisplay(this, displaySize);
    m_radialDisplay->SetPosition(displayTopLeft);
    m_radialDisplay->Hide();

    m_objectAligned = new wxCheckBox( this, wxID_ANY, "&Object Aligned", below(m_linearDisplay) + wxPoint(0,10) );

    IntSize bmpSize(28,23);
    m_gradientTypeCtrl = new BitmapListCtrl( this, to_wx(bmpSize), statusInfo, wxHORIZONTAL );
    m_gradientTypeCtrl->Add( linear_icon(bmpSize), "Linear" );
    m_gradientTypeCtrl->Add( radial_icon(bmpSize), "Radial" );
    m_gradientTypeCtrl->SetPosition(below(m_objectAligned));

    SetGradient(faint::Gradient(default_linear_gradient()));
    Connect( wxID_ANY, EVT_BITMAPLIST_SELECTION, wxCommandEventHandler(ColorPanel_Gradient_Impl::OnToggleType) );
  }

  faint::Gradient GetGradient(){
    if ( m_gradientTypeCtrl->GetSelection() == 0 ){
      faint::LinearGradient linear(m_linearDisplay->GetGradient());
      linear.SetObjectAligned(m_objectAligned->GetValue());
      return faint::Gradient(linear);
    }
    else{
      faint::RadialGradient radial(m_radialDisplay->GetGradient());
      radial.SetObjectAligned(m_objectAligned->GetValue());
      return faint::Gradient(radial);
    }
  }

  void SetGradient( const faint::Gradient& g ){
    if ( g.IsLinear() ){
      m_gradientTypeCtrl->SetSelection(0);
      const faint::LinearGradient& lg(g.GetLinear());
      m_objectAligned->SetValue( lg.GetObjectAligned() );
      m_linearDisplay->SetGradient(lg);
      m_linearDisplay->Show();
      m_radialDisplay->Hide();

    }
    else{
      m_gradientTypeCtrl->SetSelection(1);
      const faint::RadialGradient& rg(g.GetRadial());
      m_objectAligned->SetValue( rg.GetObjectAligned() );
      m_radialDisplay->SetGradient(rg);
      m_linearDisplay->Hide();
      m_radialDisplay->Show();
    }
  }
private:
  void OnToggleType( wxCommandEvent& ){
    if ( m_gradientTypeCtrl->GetSelection() == 0 ){
      faint::color_stops_t stops = m_radialDisplay->GetGradient().GetStops();
      m_linearDisplay->SetStops(stops);
      m_linearDisplay->Show();
      m_radialDisplay->Hide();
    }
    else{
      faint::color_stops_t stops = m_linearDisplay->GetGradient().GetStops();
      m_radialDisplay->SetStops(stops);
      m_linearDisplay->Hide();
      m_radialDisplay->Show();
    }
  }

  LinearGradientDisplay* m_linearDisplay;
  RadialGradientDisplay* m_radialDisplay;
  wxCheckBox* m_objectAligned;
  BitmapListCtrl* m_gradientTypeCtrl;
};

ColorPanel_Gradient::ColorPanel_Gradient( wxWindow* parent, StatusInterface& statusInfo ){
  m_impl = new ColorPanel_Gradient_Impl(parent, statusInfo);
}

ColorPanel_Gradient::~ColorPanel_Gradient(){
  m_impl = nullptr; // Deletion handled by wxWidgets
}

wxWindow* ColorPanel_Gradient::AsWindow(){
  return m_impl;
}

faint::Gradient ColorPanel_Gradient::GetGradient() const{
  return m_impl->GetGradient();
}

void ColorPanel_Gradient::SetGradient( const faint::Gradient& g ){
  m_impl->SetGradient(g);
}
