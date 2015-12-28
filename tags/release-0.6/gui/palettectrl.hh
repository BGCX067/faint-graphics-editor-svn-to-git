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
#include <map>
#include "gui/colordataobject.hh"
#include "util/palettecontainer.hh"
#include "util/settings.hh"

typedef std::map<IntPoint, faint::Color> color_map_t;
class PaletteCtrl : public wxPanel, public ColorDropTarget {
public:
  PaletteCtrl( wxWindow* parent, const Settings&, SettingNotifier&, PaletteContainer& );
  void AddColor( const faint::Color& );
  void SetBg( const faint::Color& );
  void SetFg( const faint::Color& );
  void SetPalette( const std::vector<faint::Color>& );
  void UndoSetFg();
private:
  wxDragResult OnDropColor( const IntPoint&, const faint::Color& ); // ColorDropTarget override
  bool HasColorAt(const IntPoint&) const;
  const faint::Color& GetColor(const IntPoint&) const;
  void CreateBitmap();
  void DoAddColor( const faint::Color& );
  void DoRemoveColor( const IntPoint& );
  IntSize GetButtonSize() const;
  IntSize GetButtonSpacing() const;
  IntPoint GetMaxPos();
  void Highlight( const IntPoint&, bool );
  IntPoint IndexToPalettePos( size_t );
  void DoInsertColor(const IntPoint&, const faint::Color&);
  IntPoint MousePosToPalettePos( const wxPoint& );
  void OnDoubleClick( wxMouseEvent& );
  void OnLeftDown( wxMouseEvent& );
  void OnLeftUp( wxMouseEvent& );
  void OnMotion( wxMouseEvent& );
  void OnPaint( wxPaintEvent& );
  void OnRightDown( wxMouseEvent& );
  void RemoveColor(const IntPoint& );
  void SetColor(const IntPoint&, const faint::Color&);
  SettingNotifier& m_notifier;
  const Settings& m_settings;
  faint::Color m_prevFg;
  color_map_t m_colors;
  wxBitmap m_bitmap;
  IntSize m_paletteSize;
  wxPoint m_dragStart;
  IntPoint m_highLight;
  DECLARE_EVENT_TABLE()
};

#endif
