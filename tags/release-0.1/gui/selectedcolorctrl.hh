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

#ifndef FAINT_SELECTEDCOLORCTRL_HH
#define FAINT_SELECTEDCOLORCTRL_HH
#include "wx/wx.h"
#include "colordataobject.hh"
#include "settings.hh"

// Identifiers to indicate if the foreground or background color
// should be added to the palette in the EVT_ADD_COLOR
extern const int ADD_FG;
extern const int ADD_BG;

DECLARE_EVENT_TYPE(EVT_ADD_COLOR, -1)
DECLARE_EVENT_TYPE(EVT_COPY_HEX, -1)
DECLARE_EVENT_TYPE(EVT_COPY_RGB, -1)
DECLARE_EVENT_TYPE(EVT_SWAP_COLORS, -1)

class SelectedColorCtrl : public wxWindow, public ColorDropTarget {
public:
  SelectedColorCtrl( wxWindow* parent, SettingNotifier& );
  ~SelectedColorCtrl();
  void Update( const faint::Color& fg, const faint::Color& bg );
private:
  enum which{ HIT_FG, HIT_BG, HIT_NEITHER };
  which HitTest( const wxPoint& );
  wxDragResult OnDropColor( const wxPoint&, const faint::Color& );
  void OnPaint( wxPaintEvent& );
  void OnEnter( wxMouseEvent& );
  void OnMotion( wxMouseEvent& );
  void OnLeave( wxMouseEvent& );
  void OnDoubleClick( wxMouseEvent& );
  void OnRightDown( wxMouseEvent& );
  void OnMenuChoice( wxCommandEvent& );

  faint::Color m_fgCol;
  faint::Color m_bgCol;
  wxBitmap* m_fgBmp;
  wxBitmap* m_bgBmp;
  wxRect m_fgRect;
  wxRect m_bgRect;
  which m_menuEventColor;
  SettingNotifier& m_notifier;
  DECLARE_EVENT_TABLE()
};

#endif
