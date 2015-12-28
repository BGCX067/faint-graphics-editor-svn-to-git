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
#include "wx/menu.h"
#include "wx/panel.h"
#include "bitmap/bitmap.hh"
#include "geo/geo-func.hh"
#include "geo/intrect.hh"
#include "geo/size.hh"
#include "gui/color-data-object.hh"
#include "gui/events.hh"
#include "gui/paint-dialog.hh"
#include "gui/selected-color-ctrl.hh"
#include "gui/setting-events.hh"
#include "text/formatting.hh"
#include "util/color-bitmap-util.hh"
#include "util/convert-wx.hh"
#include "util/paint.hh"

namespace faint{

// Context-menu items
static const int menu_swap = wxNewId();
static const int menu_add = wxNewId();
static const int menu_copyRgb = wxNewId();
static const int menu_copyHex = wxNewId();

static Color border_color(const Paint&){
  return color_black();
}

class SelectedColorCtrlImpl : public wxPanel, public ColorDropTarget {
public:
  typedef SelectedColorCtrl::Which Which;
  SelectedColorCtrlImpl(wxWindow* parent, const IntSize& size, StatusInterface& statusInfo)
    : wxPanel(parent, wxID_ANY),
      ColorDropTarget(this),
      m_fg(Color(0,0,0)),
      m_bg(Color(0,0,0)),
      m_menuEventColor(Which::HIT_NEITHER),
      m_statusInfo(statusInfo)
  {
    const IntPoint p0 = IntPoint::Both(0);
    const IntSize rectSize = floored(floated(size) / 1.5);
    m_fgRect = to_wx(IntRect(p0, rectSize));
    m_bgRect = to_wx(IntRect(p0 + point_from_size(rectSize / 2), rectSize));

    m_fgBmp = to_wx_bmp(paint_bitmap(m_fg, to_faint(m_fgRect.GetSize())));
    m_bgBmp = to_wx_bmp(paint_bitmap(m_bg, to_faint(m_bgRect.GetSize())));

    #ifdef __WXMSW__
    // Prevent flicker on full refresh
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    #endif
    SetInitialSize(to_wx(size));

    Bind(wxEVT_PAINT, &SelectedColorCtrlImpl::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &SelectedColorCtrlImpl::OnLeftDown, this);
    Bind(wxEVT_RIGHT_DOWN, &SelectedColorCtrlImpl::OnRightDown, this);
    Bind(wxEVT_MOTION, &SelectedColorCtrlImpl::OnMotion, this);
    Bind(wxEVT_LEAVE_WINDOW, &SelectedColorCtrlImpl::OnLeave, this);
    Bind(wxEVT_COMMAND_MENU_SELECTED, &SelectedColorCtrlImpl::OnMenuChoice, this);
  }

  void UpdateColors(const ColorChoice& colors){
    m_fg = colors.fg.first;
    m_bg = colors.bg.first;
    m_fgBmp = to_wx_bmp(with_border(paint_bitmap(m_fg, IntSize(m_fgRect.width, m_fgRect.height)), border_color(m_fg), colors.fg.second));
    m_bgBmp = to_wx_bmp(with_border(paint_bitmap(m_bg, IntSize(m_bgRect.width, m_bgRect.height)), border_color(m_bg), colors.bg.second));
    Refresh();
  }

private:
  wxBitmap GetBitmap(const Color&);
  const Paint& MenuTargetColor() const{
    // Return the color the current right click menu refers to
    return GetClickedPaint(m_menuEventColor);
  }

  const Paint& GetClickedPaint(Which hit) const{
    if (hit == Which::HIT_FG){
      return m_fg;
    }
    else if (hit == Which::HIT_BG){
      return m_bg;
    }
    assert(false);
    return m_fg;
  }

  Which HitTest(const wxPoint& pos){
    if (m_fgRect.Contains(pos)){
      return Which::HIT_FG;
    }
    else if (m_bgRect.Contains(pos)){
      return Which::HIT_BG;
    }
    else {
      return Which::HIT_NEITHER;
    }
  }

  wxDragResult OnDropColor(const IntPoint& pos, const Color& color) override{
    // Check which item the color was dropped on and report this
    // upwards.
    Which hit = HitTest(to_wx(pos));
    if (hit == Which::HIT_FG || hit == Which::HIT_NEITHER){
      SendChangeEvent(ts_Fg, Paint(color));
    }
    else if (hit == Which::HIT_BG){
      SendChangeEvent(ts_Bg, Paint(color));
    }
    return wxDragCopy;
  }

  void OnMotion(wxMouseEvent& event){
    Which hit = HitTest(event.GetPosition());

    if (hit == Which::HIT_NEITHER){
      m_statusInfo.SetMainText("");
      m_statusInfo.SetText("", 0);
    }
    else {
      m_statusInfo.SetMainText("Left click for color dialog, right click for options.");
      m_statusInfo.SetText(str(GetClickedPaint(hit)), 0);
    }
  }

  void OnPaint(wxPaintEvent&){
    wxPaintDC dc(this);
    #ifdef __WXMSW__
    // Clear the background (for wxBG_STYLE_PAINT). Not required on GTK,
    // and GetBackgroundColour gives a darker gray for some reason
    dc.SetBackground(GetBackgroundColour());
    dc.Clear();
    #endif
    dc.DrawBitmap(m_bgBmp, m_bgRect.x, m_bgRect.y);
    dc.DrawBitmap(m_fgBmp, m_fgRect.x, m_fgRect.y);
  }

  void OnLeave(wxMouseEvent&){
    m_statusInfo.SetMainText("");
  }

  void OnLeftDown(wxMouseEvent& event){
    wxPoint pos(event.GetPosition());
    Which hit = HitTest(pos);
    if (hit == Which::HIT_NEITHER){
      return;
    }
    m_statusInfo.SetMainText("");
    const Paint& src(GetClickedPaint(hit));
    wxString title(hit == Which::HIT_FG ? "Select Foreground Color" : "Select Background Color");

    if (auto picked = show_paint_dialog(0, title, src, m_statusInfo)){
      SendChangeEvent(ToSetting(hit), picked.Get());
    }
  }

  void OnRightDown(wxMouseEvent& event){
    // Show the context menu for the right clicked color
    m_statusInfo.SetMainText("");
    const wxPoint& p = event.GetPosition();
    m_menuEventColor = HitTest(p);
    wxMenu contextMenu;
    contextMenu.Append(menu_swap, "Swap colors");
    contextMenu.Append(menu_add, "Add to palette");
    contextMenu.AppendSeparator();
    contextMenu.Append(menu_copyHex, "Copy hex");
    contextMenu.Append(menu_copyRgb, "Copy rgb");

    // Disable items that require a specific color if neither the
    // foreground or background rectangles were hit
    if (m_menuEventColor == Which::HIT_NEITHER){
      contextMenu.Enable(menu_add, false);
      contextMenu.Enable(menu_copyHex, false);
      contextMenu.Enable(menu_copyRgb, false);
    }
    else{
      bool color = GetClickedPaint(m_menuEventColor).IsColor();
      if (!color){
        // Only colors can be copied as rgb or hex
        contextMenu.Enable(menu_copyHex, false);
        contextMenu.Enable(menu_copyRgb, false);
      }
    }
    PopupMenu(&contextMenu, event.GetPosition());
  }

  void OnMenuChoice(wxCommandEvent& evt){
    const int action = evt.GetId();
    if (action == menu_swap){
      wxCommandEvent newEvent(EVT_SWAP_COLORS);
      ProcessEvent(newEvent);
      return;
    }
    else{
      Paint src(MenuTargetColor());
      if (action == menu_add){
        PaintEvent newEvent(FAINT_ADD_TO_PALETTE, src);
        ProcessEvent(newEvent);
      }
      else if (action == menu_copyHex){
        assert(src.IsColor()); // Should be unavailable for other than plain color
        ColorEvent newEvent(FAINT_COPY_COLOR_HEX, src.GetColor());
        ProcessEvent(newEvent);
      }
      else if (action == menu_copyRgb){
        assert(src.IsColor()); // Should be unavailable for other than plain color
        ColorEvent newEvent(FAINT_COPY_COLOR_RGB, src.GetColor());
        ProcessEvent(newEvent);
      }
    }
  }

  void SendChangeEvent(const ColorSetting& setting, const Paint& value){
    SettingEvent<ColorSetting> event(setting, value,
      FAINT_COLOR_SETTING_CHANGE);
    event.SetEventObject(this);
    GetEventHandler()->ProcessEvent(event);
  }

  static ColorSetting ToSetting(Which hit){
    assert(hit != Which::HIT_NEITHER);
    return hit == Which::HIT_FG ?
      ts_Fg : ts_Bg;
  }

  wxRect m_fgRect;
  wxRect m_bgRect;
  Paint m_fg;
  Paint m_bg;
  wxBitmap m_fgBmp;
  wxBitmap m_bgBmp;
  Which m_menuEventColor;
  StatusInterface& m_statusInfo;
};

SelectedColorCtrl::SelectedColorCtrl(wxWindow* parent, const IntSize& size, StatusInterface& status){
  m_impl = new SelectedColorCtrlImpl(parent, size, status);
}

SelectedColorCtrl::~SelectedColorCtrl(){
  m_impl = nullptr; // Deletion is handled by wxWidgets.
}

wxWindow* SelectedColorCtrl::AsWindow(){
  return m_impl;
}

void SelectedColorCtrl::UpdateColors(const ColorChoice& colors){
  m_impl->UpdateColors(colors);
}

} // namespace
