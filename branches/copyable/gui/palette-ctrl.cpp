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
#include "wx/dir.h"
#include "bitmap/bitmap.hh"
#include "geo/measure.hh"
#include "gui/color-data-object.hh"
#include "gui/drop-source.hh"
#include "gui/events.hh"
#include "gui/mouse-capture.hh"
#include "gui/paint-dialog.hh"
#include "gui/palette-ctrl.hh"
#include "gui/setting-events.hh"
#include "text/formatting.hh"
#include "util/convert-wx.hh"

namespace faint{

static Optional<CellPos> highlight(const CellPos& pos){
  return Optional<CellPos>(pos);
}

static Optional<CellPos> no_highlight(){
  return no_option();
}

PaletteCtrl::PaletteCtrl(wxWindow* parent, const Settings& s, PaletteContainer& palettes, StatusInterface& status)
: wxPanel(parent, wxID_ANY),
  ColorDropTarget(this),
  m_mouse(this),
  m_settings(s),
  m_statusInterface(status)
{
  #ifdef __WXMSW__
  SetBackgroundStyle(wxBG_STYLE_PAINT); // Prevent flicker on full refresh
  #endif
  PaintMap& pal = palettes["default"];
  SetPalette(pal);

  Bind(wxEVT_LEAVE_WINDOW, &PaletteCtrl::OnLeaveWindow, this);
  Bind(wxEVT_LEFT_DCLICK, &PaletteCtrl::OnLeftDoubleClick, this);
  Bind(wxEVT_LEFT_DOWN, &PaletteCtrl::OnLeftDown, this);
  Bind(wxEVT_LEFT_UP, &PaletteCtrl::OnLeftUp, this);
  Bind(wxEVT_MOTION, &PaletteCtrl::OnMotion, this);
  Bind(wxEVT_PAINT, &PaletteCtrl::OnPaint, this);
  Bind(wxEVT_RIGHT_DOWN, &PaletteCtrl::OnRightDown, this);
  Bind(wxEVT_RIGHT_DCLICK, &PaletteCtrl::OnRightDown, this);
}

void PaletteCtrl::Add(const Paint& paint){
  m_paintMap.Append(paint);
  CreateBitmap(no_highlight());
}

void PaletteCtrl::CreateBitmap(const Optional<CellPos>& highlight){
  CellSize cellSize(GetButtonSize());
  CellSpacing spacing(GetButtonSpacing());
  Color bg(to_faint(GetBackgroundColour()));
  Bitmap bmp(m_paintMap.CreateBitmap(cellSize, spacing, bg));
  if (highlight.IsSet()){
    add_cell_border(bmp, highlight.Get(), cellSize, spacing);
  }
  m_bitmap = to_wx_bmp(bmp);
  SetSize(m_bitmap.GetSize());
  SetInitialSize(m_bitmap.GetSize());
  send_control_resized_event(this);
  Refresh();
  return;
}

CellSize PaletteCtrl::GetButtonSize() const{
  return CellSize(IntSize(24, 24));
}

CellSpacing PaletteCtrl::GetButtonSpacing() const{
  return CellSpacing(IntSize(2,2));
}

CellPos PaletteCtrl::MousePosToPalettePos(const IntPoint& pos) const{
  return view_to_cell_pos(pos, GetButtonSize(), GetButtonSpacing());
}

wxDragResult PaletteCtrl::OnDropColor(const IntPoint& pos, const Color& /* color */){
  // Fixme: This ignores the dropped color and only uses indexes, while assumes this panel is the source. This makes the drag and drop work for patterns etc. despite no support in ColorDataObject, but isn't proper dnd.

  bool ctrlHeld = wxGetKeyState(WXK_CONTROL);
  if (ctrlHeld){
    m_paintMap.Copy(Old(MousePosToPalettePos(to_faint(m_dragStart))),
      New(MousePosToPalettePos(pos)));
  }
  else {
    m_paintMap.Move(Old(MousePosToPalettePos(to_faint(m_dragStart))),
      New(MousePosToPalettePos(pos)));
  }
  CreateBitmap(no_highlight());
  return wxDragMove;
}

void PaletteCtrl::OnLeaveWindow(wxMouseEvent&){
  m_statusInterface.SetMainText("");
}

void PaletteCtrl::OnLeftDoubleClick(wxMouseEvent& event){
  CellPos pos = MousePosToPalettePos(to_faint(event.GetPosition()));
  if (!m_paintMap.Has(pos)){
    return;
  }
  CreateBitmap(highlight(pos));
  Optional<Paint> picked = show_paint_dialog(0, "Edit Palette Color", m_paintMap.Get(pos), m_statusInterface);
  if (picked.IsSet()){
    m_paintMap.Replace(pos, picked.Get());
    SetFg(picked.Get());
  }
  CreateBitmap(no_highlight());
}

void PaletteCtrl::OnLeftDown(wxMouseEvent& event){
  m_dragStart = event.GetPosition();
  CellPos pos = MousePosToPalettePos(to_faint(m_dragStart));
  if (!m_paintMap.Has(pos)){
    return;
  }

  Paint src(m_paintMap.Get(pos));
  SetFg(src);

  // Capture the mouse to determine distance for drag and drop
  m_mouse.Capture();
}

void PaletteCtrl::OnLeftUp(wxMouseEvent&){
  m_mouse.Release();
}

void PaletteCtrl::OnMotion(wxMouseEvent& event){
  CellPos pos = MousePosToPalettePos(to_faint(event.GetPosition()));
  if (m_paintMap.Has(pos)){
    const Paint& src = m_paintMap.Get(pos);
    m_statusInterface.SetMainText(str(src));
  }
  else {
    m_statusInterface.SetMainText("");
  }

  if (m_mouse.HasCapture()){
    const int minDistance(GetButtonSize().Get().w / 2);
    const int dragDistance = floored(distance(to_faint(event.GetPosition()),
      to_faint(m_dragStart)));
    if (dragDistance > minDistance){
      // Start drag operation
      m_mouse.Release();

      // The selected foreground color should not be changed due to
      // clicking a color for dragging.
      UndoSetFg();

      CellPos pos(MousePosToPalettePos(to_faint(m_dragStart)));
      Paint src(m_paintMap.Get(pos));
      Color c(src.IsColor() ? src.GetColor() : Color(0,0,0)); // Fixme: ColorDataObject doesn't support pattern, gradient
      ColorDataObject colorObj(c);
      FaintDropSource source(this, colorObj);
      source.CustomDoDragDrop();
    }
  }
}

void PaletteCtrl::OnPaint(wxPaintEvent&){
  wxPaintDC dc(this);
  #ifdef __WXMSW__
  // Clear the background (for wxBG_STYLE_PAINT). Not required on GTK,
  // where GetBackgroundColour also gives a darker gray for some
  // reason
  dc.SetBackground(GetBackgroundColour());
  dc.Clear();
  #endif
  dc.DrawBitmap(m_bitmap, 0, 0);
}

void PaletteCtrl::OnRightDown(wxMouseEvent& event){
  CellPos pos = MousePosToPalettePos(to_faint(event.GetPosition()));
  if (!m_paintMap.Has(pos)){
    return;
  }
  bool ctrlHeld = wxGetKeyState(WXK_CONTROL);
  if (ctrlHeld){
    m_paintMap.Erase(pos);
    CreateBitmap(no_highlight());
  }
  else {
    SetBg(m_paintMap.Get(pos));
  }
}

void PaletteCtrl::SendChangeEvent(const ColorSetting& setting, const Paint& value){
  SettingEvent<ColorSetting> event(setting, value,
    FAINT_COLOR_SETTING_CHANGE);
  event.SetEventObject(this);
  GetEventHandler()->ProcessEvent(event);
}

void PaletteCtrl::SetBg(const Paint& src){
  SendChangeEvent(ts_Bg, src);
}

void PaletteCtrl::SetFg(const Paint& src){
  m_prevFg = m_settings.Get(ts_Fg);
  SendChangeEvent(ts_Fg, src);
}

void PaletteCtrl::SetPalette(const PaintMap& paintMap){
  m_paintMap = paintMap;
  CreateBitmap(no_highlight());
}

void PaletteCtrl::UndoSetFg(){
  SendChangeEvent(ts_Fg, m_prevFg);
}

} // namespace
