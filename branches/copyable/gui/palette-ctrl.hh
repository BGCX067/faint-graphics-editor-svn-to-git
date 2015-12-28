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

#ifndef FAINT_PALETTE_CTRL_HH
#define FAINT_PALETTE_CTRL_HH
#include "wx/panel.h"
#include "gui/color-data-object.hh"
#include "gui/mouse-capture.hh"
#include "util/optional.hh"
#include "util/paint-map.hh"
#include "util/palette-container.hh"
#include "util/settings.hh"
#include "util/status-interface.hh"

namespace faint{

class PaletteCtrl : public wxPanel, public ColorDropTarget {
public:
  PaletteCtrl(wxWindow* parent,
    const Settings&,
    PaletteContainer&,
    StatusInterface&);
  void Add(const Paint&);
  void SetPalette(const PaintMap&);
private:
  void CreateBitmap(const Optional<CellPos>&);
  CellSize GetButtonSize() const;
  CellSpacing GetButtonSpacing() const;
  CellPos MousePosToPalettePos(const IntPoint&) const;
  wxDragResult OnDropColor(const IntPoint&, const Color&) override;
  void OnLeaveWindow(wxMouseEvent&);
  void OnLeftDoubleClick(wxMouseEvent&);
  void OnLeftDown(wxMouseEvent&);
  void OnLeftUp(wxMouseEvent&);
  void OnMotion(wxMouseEvent&);
  void OnPaint(wxPaintEvent&);
  void OnRightDown(wxMouseEvent&);
  void SendChangeEvent(const ColorSetting&, const Paint&);
  void SetBg(const Paint&);
  void SetFg(const Paint&);
  void UndoSetFg();
  MouseCapture m_mouse;
  const Settings& m_settings;
  Paint m_prevFg;
  wxBitmap m_bitmap;
  wxPoint m_dragStart;
  Optional<CellPos> m_highLight;
  PaintMap m_paintMap;
  StatusInterface& m_statusInterface;
};

} // namespace

#endif
