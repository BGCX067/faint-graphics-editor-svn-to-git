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
#include "colordataobject.hh"
#include "palettecontainer.hh"
#include "settings.hh"

class PaletteCtrl : public wxPanel, public ColorDropTarget {
public:
  PaletteCtrl( wxWindow* parent, const FaintSettings&, SettingNotifier&, PaletteContainer& );
  void AddColor( const faint::Color& );
  void SetBg( const faint::Color& );
  void SetFg( const faint::Color& );
  void UndoSetFg();
  void Clear();
private:
  wxDragResult OnDropColor( const wxPoint&, const faint::Color& );
  void OnRemoveColor( wxCommandEvent& );
  SettingNotifier& m_notifier;
  const FaintSettings& m_settings;
  faint::Color m_prevFg;
  wxGridSizer* m_sizer;
  DECLARE_EVENT_TABLE()
};

class ColorButton : public wxWindow {
public:
  ColorButton( wxWindow* parent, const faint::Color& );
  ~ColorButton();
private:
  PaletteCtrl* GetParentPalette();
  void SetColor( const faint::Color& );
  void OnPaint( wxPaintEvent& );
  void OnMotion( wxMouseEvent& );
  void OnLeave( wxMouseEvent& );
  void OnLeftDown( wxMouseEvent& );
  void OnLeftUp( wxMouseEvent& );
  void OnDoubleClick( wxMouseEvent& );
  void OnRightDown( wxMouseEvent& );

  faint::Color m_color;
  wxBitmap* m_bitmap;
  wxPoint m_dragStart;
  DECLARE_EVENT_TABLE()
};

#endif
