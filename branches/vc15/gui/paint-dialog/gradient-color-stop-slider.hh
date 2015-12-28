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

#include "util-wx/make-event.hh"

namespace faint {

const int g_sliderHeight = 40;

static void draw_cross(Bitmap& bmp, const IntRect& r){
  int x0 = r.Left() + 2;
  int xMid = r.Left() + r.w / 2;
  int x1 = r.Right() - 2;
  int y0 = r.Top() + 2;
  int yMid = r.Top() + r.h / 2;
  int y1 = r.Bottom() - 2;

  LineSettings s(color_black(), 1, LineStyle::SOLID, LineCap::BUTT);
  draw_line(bmp, {{x0, yMid}, {x1, yMid}}, s);
  draw_line(bmp, {{xMid, y0}, {xMid, y1}}, s);
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

class ColorStopsRegion{
public:
  ColorStopsRegion(){
  }

  ColorStopsRegion(const color_stops_t& stops,
    const IntSize& sz,
    const wxColour& bgColor)
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
      draw_rect(bmp, addRect, {color_black(), 1, LineStyle::SOLID});
      draw_cross(bmp, addRect);

      const double offset = prevStop.GetOffset() +
        (s.GetOffset() - prevStop.GetOffset()) / 2;
      m_regions.push_back(std::make_pair(addRect,
          HandleHitInfo::HitAddHandle(offset , prevStop.GetColor())));
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
      BorderSettings outline(color_black(), 1, LineStyle::SOLID);
      fill_polygon(bmp, pts, fill);
      draw_polygon(bmp, pts, outline);
      int ry0 = sz.h / 2 + 2;
      int ry1= std::min(sz.h, ry0 + 2 * w + 1);
      m_regions.push_back(std::make_pair(IntRect(IntPoint(cx-w,0),
        IntPoint(cx+w, sz.h / 2 - 1)),
        HandleHitInfo::HitMoveRegion(resigned(i), s)));

      IntRect colorRect(IntPoint(cx - w, ry0), IntPoint(cx + w, ry1));
      fill_rect(bmp, colorRect, fill);
      draw_rect(bmp, colorRect, outline);
      m_regions.emplace_back(colorRect,
        HandleHitInfo::HitColorChange(resigned(i), s));
    }
    m_bmp = to_wx_bmp(bmp);
  }

  const wxBitmap& GetBitmap(){
    return m_bmp;
  }

  HandleHitInfo HitTest(const IntPoint& pos){
    // Traverse the regions in reverse so that the move-handles have
    // higher priority than the add-buttons
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

MAKE_FAINT_COMMAND_EVENT(GRADIENT_SLIDER_CHANGE);

template<typename T>
class ColorStopSlider : public wxPanel {
public:
  ColorStopSlider(wxWindow* parent,
    wxSize size,
    T& gradient,
    DialogContext& dialogContext)
    : wxPanel(parent, wxID_ANY),
      m_dialogContext(dialogContext),
      m_gradient(gradient),
      m_handle(-1),
      m_mouse(this)
  {
    SetInitialSize(size);
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_LEFT_DOWN,
      [this](wxMouseEvent& event){
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
      });

    Bind(wxEVT_LEFT_UP,
      [this](wxMouseEvent&){
        m_mouse.Release();
      });

    Bind(wxEVT_MOTION,
      [this](wxMouseEvent& event){
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
            if (hit.type == HandleHitInfo::MOVE_HANDLE ||
              hit.type == HandleHitInfo::ADD_HANDLE ||
              event.ControlDown())
            {
              SetCursor(wxCURSOR_SIZEWE);
            }
            else{
              SetCursor(wxCURSOR_HAND);
            }
          }
        }
      });

    Bind(wxEVT_PAINT,
      [this](wxPaintEvent&){
        wxPaintDC dc(this);
        dc.DrawBitmap(m_region.GetBitmap(), 0, 0);
      });

    Bind(wxEVT_RIGHT_DOWN,
      [this](wxMouseEvent& event){
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
      });

    UpdateGradient();
  }

  void UpdateGradient(){
    m_region = ColorStopsRegion(m_gradient.GetStops(),
      to_faint(GetSize()),
      GetBackgroundColour());
  }

private:
  void PickHandleColor(index_t handle){
    ColorStop stop(m_gradient.GetStop(handle));
    Optional<Color> c = show_color_only_dialog(this,
      "Select Stop Color", stop.GetColor(),
      m_dialogContext);
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
    coord x = constrained(Min(0.0),
      mousePos.x / floated(GetSize().GetWidth()), Max(1.0));
    ColorStop stop(get_stop_color(index_t(m_handle), m_gradient), x);
    m_gradient.SetStop(index_t(m_handle), stop);
    UpdateGradient();
    SendChangeEvent();
    Refresh();
  }

  DialogContext& m_dialogContext;
  T& m_gradient;
  int m_handle;
  MouseCapture m_mouse;
  ColorStopsRegion m_region;
};

using LinearSlider = ColorStopSlider<LinearGradient>;
using RadialSlider = ColorStopSlider<RadialGradient>;

} // namespace
