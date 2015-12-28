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

#ifndef FAINT_PALETTECTRL_HH
#define FAINT_PALETTECTRL_HH
#include "wx/wx.h"
#include "gui/colordataobject.hh"
#include "util/draw-source-map.hh"
#include "util/palettecontainer.hh"
#include "util/settings.hh"
#include "util/statusinterface.hh"

class PaletteCtrl : public wxPanel, public ColorDropTarget {
public:
  PaletteCtrl( wxWindow* parent,
    const Settings&,
    SettingNotifier&,
    PaletteContainer&,
    StatusInterface& );
  void Add( const faint::DrawSource& );
  void SetPalette( const faint::DrawSourceMap& );
private:
  void CreateBitmap( const Optional<faint::CellPos>& );
  faint::CellSize GetButtonSize() const;
  faint::CellSpacing GetButtonSpacing() const;
  faint::CellPos MousePosToPalettePos( const IntPoint& ) const;
  void OnCaptureLost( wxMouseCaptureLostEvent& );
  wxDragResult OnDropColor( const IntPoint&, const faint::Color& ) override;
  void OnLeaveWindow( wxMouseEvent& );
  void OnLeftDoubleClick( wxMouseEvent& );
  void OnLeftDown( wxMouseEvent& );
  void OnLeftUp( wxMouseEvent& );
  void OnMotion( wxMouseEvent& );
  void OnPaint( wxPaintEvent& );
  void OnRightDown( wxMouseEvent& );
  void SetBg( const faint::DrawSource& );
  void SetFg( const faint::DrawSource& );
  void UndoSetFg();
  SettingNotifier& m_notifier;
  const Settings& m_settings;
  faint::DrawSource m_prevFg;
  wxBitmap m_bitmap;
  wxPoint m_dragStart;
  Optional<faint::CellPos> m_highLight;
  faint::DrawSourceMap m_drawSourceMap;
  StatusInterface& m_statusInterface;
  DECLARE_EVENT_TABLE()
};

#endif
