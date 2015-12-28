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
#include <cmath>
#include "wx/bitmap.h"
#include "wx/checkbox.h"
#include "wx/dcclient.h"
#include "wx/stattext.h"
#include "wx/textctrl.h"
#include "bitmap/bitmap.hh"
#include "bitmap/draw.hh"
#include "geo/adjust.hh"
#include "geo/geo-func.hh"
#include "geo/intrect.hh"
#include "geo/intsize.hh"
#include "geo/line.hh"
#include "gui/bitmap-list-ctrl.hh"
#include "gui/layout.hh"
#include "gui/mouse-capture.hh"
#include "gui/paint-dialog.hh"
#include "gui/paint-dialog/gradient-panel.hh"
#include "gui/placement.hh"
#include "text/formatting.hh"
#include "util/color-bitmap-util.hh"
#include "util/convert-wx.hh"
#include "util/gui-util.hh"
#include "util/iter.hh"
#include "util/pattern.hh"

namespace faint{

static Bitmap with_angle_indicator(const Bitmap& src, const Angle& angle){
  IntSize sz(src.GetSize());
  int dx = static_cast<int>(cos(-angle) * sz.w);
  int dy = static_cast<int>(sin(-angle) * sz.w);
  int cx = sz.w / 2;
  int cy = sz.h / 2;
  Bitmap dst(copy(src));
  Color border(50,50,50);
  Color fill(255,255,255);

  IntPoint start(cx, cy);
  IntPoint end(cx + dx, cy + dy);
  draw_line(dst, {start, end}, {border, 3, false, LineCap::BUTT});
  draw_line(dst, {start, end}, {fill, 1, false, LineCap::BUTT});
  put_pixel(dst, start, border);
  return dst;
}

static LinearGradient default_linear_gradient(){
  return LinearGradient(Angle::Zero(),
    {{{255,0,0}, 0.0}, {{0,0,0}, 1.0}});
}

template<typename T>
static Color get_stop_color(const index_t& index, const T& g){
  return g.GetStop(index).GetColor();
}

class HandleHitInfo{
public:
  enum HitType{
    MISS,
    MOVE_HANDLE,
    CHANGE_HANDLE_COLOR,
    ADD_HANDLE
  };

  static HandleHitInfo Miss(){
    return HandleHitInfo(MISS, 0, 0.0, color_white());
  }

  static HandleHitInfo HitMoveRegion(int index, const ColorStop& s){
    return HandleHitInfo(MOVE_HANDLE, index, s.GetOffset(), s.GetColor());
  }

  static HandleHitInfo HitColorChange(int index, const ColorStop& s){
    return HandleHitInfo(CHANGE_HANDLE_COLOR, index, s.GetOffset(), s.GetColor());
  }

  static HandleHitInfo HitAddHandle(double offset, const Color& c){
    return HandleHitInfo(ADD_HANDLE, 0, offset, c);
  }

  HitType type;
  int index;
  double offset;
  Color color;
private:
  HandleHitInfo(HitType in_type, int in_index, double in_offset, const Color& in_color) :
    type(in_type),
    index(in_index),
    offset(in_offset),
    color(in_color)
  {}
};

void draw_cross(Bitmap& bmp, const IntRect& r){
  int x0 = r.Left() + 2;
  int xMid = r.Left() + r.w / 2;
  int x1 = r.Right() - 2;
  int y0 = r.Top() + 2;
  int yMid = r.Top() + r.h / 2;
  int y1 = r.Bottom() - 2;

  LineSettings s(color_black(), 1, false, LineCap::BUTT);
  draw_line(bmp, {{x0, yMid}, {x1, yMid}}, s);
  draw_line(bmp, {{xMid, y0}, {xMid, y1}}, s);
}

class ColorStopsRegion{
public:
  ColorStopsRegion(){
  }

  ColorStopsRegion(const color_stops_t& stops, const IntSize& sz, const wxColour& bgColor)
  {
    Bitmap bmp(sz, to_faint(bgColor));
    int w = 5;

    // Add the regions for creating new handles
    color_stops_t sortedStops(stops);
    std::sort(begin(sortedStops), end(sortedStops),
      [](const ColorStop& s1, const ColorStop& s2){
        return s1.GetOffset() < s2.GetOffset();
      });
    for (size_t i = 1; i < sortedStops.size(); i++){
      const ColorStop& s = sortedStops[i];
      int cx = 5 + floored((sz.w - 11) * s.GetOffset());
      const ColorStop& prevStop(sortedStops[i-1]);
      int prevX = w + floored((sz.w - 11) * prevStop.GetOffset());
      int midX = prevX + (cx - prevX) / 2;
      int y1 = std::min(2 * w, sz.h / 2);

      IntRect addRect(IntPoint(midX - w, 2), IntPoint(midX + w, y1 + 2));
      draw_rect(bmp, addRect, {color_black(), 1, false});
      draw_cross(bmp, addRect);

      const double offset = prevStop.GetOffset() + (s.GetOffset() - prevStop.GetOffset()) / 2;
      m_regions.push_back(std::make_pair(addRect, HandleHitInfo::HitAddHandle(offset , prevStop.GetColor())));
    }

    // Add the regions for moving a handle or changing its color
    for (size_t i = 0; i != stops.size(); i++){
      const ColorStop& s(stops[i]);
      int cx = 5 + floored((sz.w - 11) * s.GetOffset());
      std::vector<IntPoint> pts;
      pts.emplace_back(cx, 0);
      pts.emplace_back(cx + w, sz.h / 2 - 1);
      pts.emplace_back(cx - w, sz.h / 2 - 1);

      // Use a bitmap for the fill, so that the color stop handles
      // indicate alpha with a checkered background
      Paint fill(Pattern(color_bitmap(s.GetColor(), IntSize(6,6))));
      BorderSettings outline(color_black(), 1, false);
      fill_polygon(bmp, pts, fill);
      draw_polygon(bmp, pts, outline);
      int ry0 = sz.h / 2 + 2;
      int ry1= std::min(sz.h, ry0 + 2 * w + 1);
      m_regions.push_back(std::make_pair(IntRect(IntPoint(cx-w,0), IntPoint(cx+w, sz.h / 2 - 1)), HandleHitInfo::HitMoveRegion(resigned(i), s)));

      IntRect colorRect(IntPoint(cx - w, ry0), IntPoint(cx + w, ry1));
      fill_rect(bmp, colorRect, fill);
      draw_rect(bmp, colorRect, outline);
      m_regions.emplace_back(colorRect, HandleHitInfo::HitColorChange(resigned(i), s));
    }
    m_bmp = to_wx_bmp(bmp);
  }

  const wxBitmap& GetBitmap(){
    return m_bmp;
  }

  HandleHitInfo HitTest(const IntPoint& pos){
    // Traverse the regions in reverse so that the move-handles
    // have higher priority than the add-buttons
    for (auto region : reversed(m_regions)){
      if (region.first.Contains(pos)){
        return region.second;
      }
    }
    return HandleHitInfo::Miss();
  }

private:
  std::vector<std::pair<IntRect, HandleHitInfo> > m_regions;
  wxBitmap m_bmp;
};

DEFINE_EVENT_TYPE(EVT_GRADIENT_SLIDER_CHANGE)

template<typename T>
class ColorStopSlider : public wxPanel {
public:
  ColorStopSlider(wxWindow* parent, wxSize size, T& gradient)
    : wxPanel(parent, wxID_ANY),
      m_gradient(gradient),
      m_handle(-1),
      m_mouse(this)
  {
    SetInitialSize(size);
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_LEFT_DOWN, &ColorStopSlider<T>::OnLeftDown, this);
    Bind(wxEVT_LEFT_UP, &ColorStopSlider<T>::OnLeftUp, this);
    Bind(wxEVT_MOTION, &ColorStopSlider<T>::OnMotion, this);
    Bind(wxEVT_PAINT, &ColorStopSlider<T>::OnPaint, this);
    Bind(wxEVT_RIGHT_DOWN, &ColorStopSlider<T>::OnRightDown, this);
    UpdateGradient();
  }

  void UpdateGradient(){
    m_region = ColorStopsRegion(m_gradient.GetStops(), to_faint(GetSize()), GetBackgroundColour());
  }

private:
  void OnLeftDown(wxMouseEvent& event){
    IntPoint pos(to_faint(event.GetPosition()));
    HandleHitInfo hit = m_region.HitTest(pos);
    m_handle = hit.index;
    if (hit.type == HandleHitInfo::MISS){
      return;
    }

    if (hit.type == HandleHitInfo::ADD_HANDLE){
      m_gradient.Add(ColorStop(hit.color, hit.offset));
      m_handle = m_gradient.GetNumStops().Get() - 1;
      m_mouse.Capture();
      SetHandleFromPos(to_faint(event.GetPosition()));
      return;
    }

    if (!event.ControlDown()){
      if (hit.type == HandleHitInfo::CHANGE_HANDLE_COLOR){
        PickHandleColor(index_t(m_handle));
        return;
      }
    }
    else {
      // Clone the handle
      m_gradient.Add(m_gradient.GetStop(index_t(m_handle)));
      m_handle = m_gradient.GetNumStops().Get() - 1;
    }
    m_mouse.Capture();
    SetHandleFromPos(to_faint(event.GetPosition()));
  }

  void OnLeftUp(wxMouseEvent&){
    m_mouse.Release();
  }

  void OnMotion(wxMouseEvent& event){
    if (m_mouse.HasCapture()){
      SetHandleFromPos(to_faint(event.GetPosition()));
    }
    else {
      HandleHitInfo hit = m_region.HitTest(to_faint(event.GetPosition()));
      m_handle = hit.index;
      if (hit.type == HandleHitInfo::MISS){
        SetCursor(wxCURSOR_ARROW);
      }
      else{
        if (hit.type == HandleHitInfo::MOVE_HANDLE || hit.type == HandleHitInfo::ADD_HANDLE || event.ControlDown()){
          SetCursor(wxCURSOR_SIZEWE);
        }
        else{
          SetCursor(wxCURSOR_HAND);
        }
      }
    }
  }

  void OnPaint(wxPaintEvent&){
    wxPaintDC dc(this);
    dc.DrawBitmap(m_region.GetBitmap(), 0, 0);
  }

  void OnRightDown(wxMouseEvent& event){
    HandleHitInfo hit = m_region.HitTest(to_faint(event.GetPosition()));
    m_handle = hit.index;
    if (hit.type == HandleHitInfo::MISS){
      return;
    }
    if (hit.type == HandleHitInfo::ADD_HANDLE){
      return;
    }

    if (m_gradient.GetNumStops() > 1){
      RemoveHandle(index_t(m_handle));
    }
  }

  void PickHandleColor(index_t handle){
    ColorStop stop(m_gradient.GetStop(handle));
    Optional<Color> c = show_color_only_dialog(this, "Select Stop Color", stop.GetColor());
    if (c.IsSet()){
      m_gradient.SetStop(handle, ColorStop(c.Get(), stop.GetOffset()));
      UpdateGradient();
      SendChangeEvent();
      Refresh();
    }
  }

  void RemoveHandle(index_t handle){
    m_gradient.Remove(handle);
    UpdateGradient();
    SendChangeEvent();
    Refresh();
  }

  void SendChangeEvent(){
    wxCommandEvent changeEvent(EVT_GRADIENT_SLIDER_CHANGE, GetId());
    GetEventHandler()->ProcessEvent(changeEvent);
  }

  void SetHandleFromPos(const IntPoint& mousePos){
    coord x = constrained(Min(0.0), mousePos.x / floated(GetSize().GetWidth()), Max(1.0));
    ColorStop stop(get_stop_color(index_t(m_handle), m_gradient), x);
    m_gradient.SetStop(index_t(m_handle), stop);
    UpdateGradient();
    SendChangeEvent();
    Refresh();
  }

  T& m_gradient;
  int m_handle;
  MouseCapture m_mouse;
  ColorStopsRegion m_region;
};

typedef ColorStopSlider<LinearGradient> LinearSlider;
typedef ColorStopSlider<RadialGradient> RadialSlider;
static const int g_sliderHeight = 40;

DECLARE_EVENT_TYPE(EVT_GRADIENT_ANGLE_PICKED, -1)
DEFINE_EVENT_TYPE(EVT_GRADIENT_ANGLE_PICKED)

class LinearGradientDisplay : public wxPanel {
public:
  LinearGradientDisplay(wxWindow* parent, const wxSize& size):
    wxPanel(parent, wxID_ANY),
    m_mouse(this),
    m_offset(5,0)
  {
    SetBackgroundStyle(wxBG_STYLE_PAINT); // Prevent flicker on full refresh
    SetInitialSize(size);
    m_slider = new LinearSlider(this, wxSize(size.GetWidth(), g_sliderHeight), m_gradient);
    m_slider->SetPosition(wxPoint(0, size.GetHeight() - g_sliderHeight));
    SetCursor(to_wx_cursor(Cursor::CROSSHAIR));
    Bind(wxEVT_PAINT, &LinearGradientDisplay::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &LinearGradientDisplay::OnLeftDown, this);
    Bind(wxEVT_LEFT_UP, &LinearGradientDisplay::OnLeftUp, this);
    Bind(wxEVT_MOTION, &LinearGradientDisplay::OnMotion, this);
    Bind(EVT_GRADIENT_SLIDER_CHANGE, &LinearGradientDisplay::OnSliderChange, this);
  }

  LinearGradient GetGradient() const{
    return m_gradient;
  }

  bool SetBackgroundColour(const wxColour& bgColor) override{
    wxPanel::SetBackgroundColour(bgColor);
    m_slider->SetBackgroundColour(bgColor);
    UpdateBitmap();
    return true;
  }

  void SetGradient(const LinearGradient& g){
    m_gradient = g;
    UpdateBitmap();
    m_slider->UpdateGradient();
    Refresh();
  }

  void SetStops(const color_stops_t& stops){
    m_gradient.SetStops(stops);
    m_slider->UpdateGradient();
    UpdateBitmap();
    Refresh();
  }

private:
  void OnLeftDown(wxMouseEvent& event){
    int max_x = m_bmp.GetWidth();
    int max_y = m_bmp.GetHeight();
    wxPoint pos(event.GetPosition());
    if (pos.x > max_x || pos.y > max_y){
      return;
    }
    m_mouse.Capture();
    SetAngleFromPos(GetBitmapPos(event));
  }

  void OnLeftUp(wxMouseEvent&){
    m_mouse.Release();
  }

  void OnMotion(wxMouseEvent& event){
    if (m_mouse.HasCapture()){
      SetAngleFromPos(GetBitmapPos(event));
    }
  }

  void OnPaint(wxPaintEvent&){
    wxPaintDC dc(this);
    dc.DrawBitmap(m_bmp, 0, 0);
  }

  void OnSliderChange(wxEvent&){
    UpdateBitmap();
    Refresh();
  }

  IntPoint GetBitmapPos(const wxMouseEvent& event){
    const IntPoint pos(to_faint(event.GetPosition()) - m_offset);
    return event.ShiftDown() ? floored(adjust_to_45(floated(point_from_size(m_gradientBmp.GetSize() / 2)), floated(pos))) :
      pos;
  }

  void SetAngleFromPos(const IntPoint& mousePos){
    IntPoint pos = mousePos - point_from_size(m_gradientBmp.GetSize() / 2);
    if (pos.x == 0 && pos.y == 0){
      return;
    }
    Angle angle = -atan2(static_cast<double>(pos.y), static_cast<double>(pos.x));

    m_gradient.SetAngle(angle);
    UpdateBitmap();
    Refresh();
    wxCommandEvent event(EVT_GRADIENT_ANGLE_PICKED, GetId());
    event.SetEventObject(this);
    GetEventHandler()->ProcessEvent(event);
  }

  void UpdateBitmap(){
    IntSize sz(to_faint(GetSize()));
    Bitmap bg(sz, to_faint(GetBackgroundColour()));
    m_gradientBmp = gradient_bitmap(Gradient(unrotated(m_gradient)), sz - IntSize(10,g_sliderHeight)); // Fixme: Literals
    blit(with_border(with_angle_indicator(m_gradientBmp, m_gradient.GetAngle())), onto(bg), m_offset);
    m_bmp = to_wx_bmp(bg);
  }

  wxBitmap m_bmp;
  LinearGradient m_gradient;
  Bitmap m_gradientBmp;
  MouseCapture m_mouse;
  LinearSlider* m_slider;
  const IntPoint m_offset;
};

class RadialGradientDisplay : public wxPanel {
public:
  RadialGradientDisplay(wxWindow* parent, const wxSize& size)
    : wxPanel(parent, wxID_ANY)
  {
    SetBackgroundStyle(wxBG_STYLE_PAINT); // Prevent flicker on full refresh
    SetInitialSize(size);
    m_slider = new RadialSlider(this, wxSize(size.GetWidth(), g_sliderHeight), m_gradient);
    m_slider->SetPosition(wxPoint(0, size.GetHeight() - g_sliderHeight)); // Fixme

    Bind(EVT_GRADIENT_SLIDER_CHANGE, &RadialGradientDisplay::OnSliderChange, this);
    Bind(wxEVT_PAINT, &RadialGradientDisplay::OnPaint, this);
  }

  RadialGradient GetGradient() const{
    return m_gradient;
  }

  void SetGradient(const RadialGradient& g){
    m_gradient = g;
    UpdateBitmap();
    m_slider->UpdateGradient();
    Refresh();
  }

  void SetStops(const color_stops_t& stops){
    m_gradient.SetStops(stops);
    UpdateBitmap();
    m_slider->UpdateGradient();
    Refresh();
  }

  bool SetBackgroundColour(const wxColour& bgColor) override{
    wxPanel::SetBackgroundColour(bgColor);
    m_slider->SetBackgroundColour(bgColor);
    UpdateBitmap();
    return true;
  }
private:
  void OnPaint(wxPaintEvent& event){
    wxPaintDC dc(this);
    dc.DrawBitmap(m_bmp, 0, 0);
    event.Skip();
  }

  void OnSliderChange(wxEvent&){
    UpdateBitmap();
    Refresh();
  }

  void UpdateBitmap(){
    IntSize sz(to_faint(GetSize()));
    Bitmap bg(sz - IntSize(0, g_sliderHeight), to_faint(GetBackgroundColour()));

    IntSize gSz(sz - IntSize(10, g_sliderHeight));
    m_gradientBmp = subbitmap(gradient_bitmap(Gradient(m_gradient), IntSize(gSz.w * 2, gSz.h)),
      IntRect(IntPoint(gSz.w,0), gSz));
    blit(with_border(m_gradientBmp), onto(bg), IntPoint(5,0));
    m_bmp = to_wx_bmp(bg);
  }

  wxBitmap m_bmp;
  RadialGradient m_gradient;
  Bitmap m_gradientBmp;
  RadialSlider* m_slider;
};

static color_stops_t icon_color_stops(){
  return {
    {color_black(), 0.0},
    {color_white(), 1.0}};
}

static wxBitmap linear_icon(const IntSize& size){
  LinearGradient g(Angle::Zero(), icon_color_stops());
  Bitmap bmp(gradient_bitmap(Gradient(g), size));
  return to_wx_bmp(bmp);
}

static wxBitmap radial_icon(const IntSize& size){
  Radii radii(floated(size.w), floated(size.h));
  RadialGradient g(Point(0,0), radii, icon_color_stops());
  Bitmap bmp(gradient_bitmap(Gradient(g), size));
  return to_wx_bmp(bmp);
}

class ColorPanel_Gradient_Impl : public wxPanel{
public:
  ColorPanel_Gradient_Impl(wxWindow* parent, StatusInterface& statusInfo)
    : wxPanel(parent, wxID_ANY)
  {
    const wxPoint displayTopLeft(panel_padding, panel_padding);

    IntSize bmpSize(28,23);
    m_gradientTypeCtrl = new BitmapListCtrl(this, to_wx(bmpSize), statusInfo, wxVERTICAL);
    m_gradientTypeCtrl->Add(linear_icon(bmpSize), "Linear");
    m_gradientTypeCtrl->Add(radial_icon(bmpSize), "Radial");
    m_gradientTypeCtrl->SetPosition(displayTopLeft);

    const wxSize displaySize(420 - m_gradientTypeCtrl->GetSize().GetWidth() - panel_padding,100);

    m_linearDisplay = new LinearGradientDisplay(this, displaySize);
    m_linearDisplay->SetPosition(to_the_right_of(m_gradientTypeCtrl));
    m_linearDisplay->Hide();

    m_radialDisplay = new RadialGradientDisplay(this, displaySize);
    m_radialDisplay->SetPosition(to_the_right_of(m_gradientTypeCtrl));
    m_radialDisplay->Hide();

    m_angleTextLabel = new wxStaticText(this, wxID_ANY, "&Angle",wxPoint(panel_padding, bottom(m_linearDisplay) + 10));

    m_angleTextCtrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(50, -1));
    m_angleTextCtrl->SetPosition(to_the_right_middle_of(m_angleTextLabel, m_angleTextCtrl->GetSize()));
    m_angleTextCtrl->Bind(wxEVT_KILL_FOCUS, &ColorPanel_Gradient_Impl::OnTextCtrlKillFocus, this);

    m_objectAligned = new wxCheckBox(this, wxID_ANY, "&Object Aligned", below(m_angleTextLabel) + wxPoint(0,10));

    SetGradient(Gradient(default_linear_gradient()));
    Bind(EVT_BITMAPLIST_SELECTION, &ColorPanel_Gradient_Impl::OnToggleType, this);
    Bind(EVT_GRADIENT_ANGLE_PICKED, &ColorPanel_Gradient_Impl::OnAnglePicked, this);
    Bind(wxEVT_COMMAND_TEXT_UPDATED, &ColorPanel_Gradient_Impl::OnTextEntry, this);
  }

  Gradient GetGradient(){
    if (m_gradientTypeCtrl->GetSelection() == 0){
      LinearGradient linear(m_linearDisplay->GetGradient());
      linear.SetObjectAligned(m_objectAligned->GetValue());
      return Gradient(linear);
    }
    else{
      RadialGradient radial(m_radialDisplay->GetGradient());
      radial.SetObjectAligned(m_objectAligned->GetValue());
      return Gradient(radial);
    }
  }

  void SetGradient(const Gradient& g){
    if (g.IsLinear()){
      m_gradientTypeCtrl->SetSelection(0);
      const LinearGradient& lg(g.GetLinear());
      m_objectAligned->SetValue(lg.GetObjectAligned());
      m_linearDisplay->SetGradient(lg);
      m_linearDisplay->Show();
      m_radialDisplay->Hide();
      UpdateAngleText();
      m_angleTextLabel->Enable(true);
      m_angleTextCtrl->Enable(true);

    }
    else{
      m_gradientTypeCtrl->SetSelection(1);
      const RadialGradient& rg(g.GetRadial());
      m_objectAligned->SetValue(rg.GetObjectAligned());
      m_radialDisplay->SetGradient(rg);
      m_linearDisplay->Hide();
      m_radialDisplay->Show();
      m_angleTextLabel->Enable(false);
      m_angleTextCtrl->Enable(false);
    }
  }

  bool SetBackgroundColour(const wxColour& bgColor) override{
    wxPanel::SetBackgroundColour(bgColor);
    m_linearDisplay->SetBackgroundColour(bgColor);
    m_radialDisplay->SetBackgroundColour(bgColor);
    return true;
  }

private:

  void OnAnglePicked(wxEvent&){
        UpdateAngleText();
  }

  void OnTextEntry(wxCommandEvent& evt){
    wxTextCtrl* ctrl = dynamic_cast<wxTextCtrl*>(evt.GetEventObject());
    m_linearDisplay->SetGradient(with_angle(m_linearDisplay->GetGradient(),
      Angle::Deg(parse_coord_value(ctrl, 0.0))));
  }

  void OnTextCtrlKillFocus(wxFocusEvent& e){
    // Propagate event to clear selection indicator etc.
    e.Skip();
    m_angleTextCtrl->ChangeValue(to_wx(str_degrees(m_linearDisplay->GetGradient().GetAngle())));
  }

  void OnToggleType(wxEvent&){
    if (m_gradientTypeCtrl->GetSelection() == 0){
      color_stops_t stops = m_radialDisplay->GetGradient().GetStops();
      m_linearDisplay->SetStops(stops);
      m_linearDisplay->Show();
      m_radialDisplay->Hide();
      m_angleTextLabel->Enable(true);
      m_angleTextCtrl->Enable(true);
    }
    else{
      color_stops_t stops = m_linearDisplay->GetGradient().GetStops();
      m_radialDisplay->SetStops(stops);
      m_linearDisplay->Hide();
      m_radialDisplay->Show();
      m_angleTextLabel->Enable(false);
      m_angleTextCtrl->Enable(false);
    }
  }

  void UpdateAngleText(){
    m_angleTextCtrl->ChangeValue(to_wx(str_degrees(m_linearDisplay->GetGradient().GetAngle())));
  }

  wxStaticText* m_angleTextLabel;
  wxTextCtrl* m_angleTextCtrl;
  LinearGradientDisplay* m_linearDisplay;
  RadialGradientDisplay* m_radialDisplay;
  wxCheckBox* m_objectAligned;
  BitmapListCtrl* m_gradientTypeCtrl;
};

ColorPanel_Gradient::ColorPanel_Gradient(wxWindow* parent, StatusInterface& statusInfo){
  m_impl = new ColorPanel_Gradient_Impl(parent, statusInfo);
}

ColorPanel_Gradient::~ColorPanel_Gradient(){
  m_impl = nullptr; // Deletion handled by wxWidgets
}

wxWindow* ColorPanel_Gradient::AsWindow(){
  return m_impl;
}

Gradient ColorPanel_Gradient::GetGradient() const{
  return m_impl->GetGradient();
}

void ColorPanel_Gradient::SetGradient(const Gradient& g){
  m_impl->SetGradient(g);
}

} // namespace
